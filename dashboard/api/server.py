from __future__ import annotations

import json
import os
import re
import signal
import subprocess
import threading
import time
import uuid
from collections import deque
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional

from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field, field_validator

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
BOXFORGE_BIN = Path(os.environ.get("BOXFORGE_BIN", REPO / "boxforge")).resolve()
DEFAULT_ROOTFS = Path(os.environ.get("BOXFORGE_ROOTFS", REPO / "rootfs" / "busybox")).resolve()
CGROUP_ROOT = Path("/sys/fs/cgroup")
ALLOWED_ROOTS = [REPO.resolve()]
for extra in os.environ.get("BOXFORGE_ALLOWED_ROOTS", "").split(os.pathsep):
    if extra.strip():
        ALLOWED_ROOTS.append(Path(extra).expanduser().resolve())

app = FastAPI(title="BoxForge Studio API", version="0.1.0", docs_url="/api/docs", openapi_url="/api/openapi.json")
app.add_middleware(CORSMiddleware, allow_origins=["http://127.0.0.1:5173", "http://localhost:5173"], allow_methods=["GET", "POST"], allow_headers=["Content-Type"])

_lock = threading.RLock()
_containers: dict[str, dict] = {}
_events = deque(maxlen=250)
_event_seq = 0

def now() -> str:
    return datetime.now(timezone.utc).isoformat()

def event(message: str, target: str = "BoxForge runtime", kind: str = "info") -> None:
    global _event_seq
    with _lock:
        _event_seq += 1
        _events.appendleft({"id": _event_seq, "time": now(), "message": message, "target": target, "kind": kind})

def under_allowed_root(path: Path) -> bool:
    return any(path == root or root in path.parents for root in ALLOWED_ROOTS)

def safe_rootfs(raw: str) -> Path:
    candidate = Path(raw).expanduser()
    if not candidate.is_absolute():
        candidate = (REPO / candidate).resolve()
    else:
        candidate = candidate.resolve()
    if candidate == Path("/") or not candidate.is_dir():
        raise HTTPException(400, "Rootfs must be an existing directory other than /.")
    if not under_allowed_root(candidate):
        raise HTTPException(400, "Rootfs must be inside the repository or BOXFORGE_ALLOWED_ROOTS.")
    return candidate

def parse_memory(raw: Optional[str]) -> Optional[str]:
    if raw is None or raw == "":
        return None
    if not re.fullmatch(r"[1-9][0-9]*(?:K|KB|M|MB|G|GB|KiB|MiB|GiB)?", raw, re.I):
        raise HTTPException(400, "Invalid memory limit. Examples: 64M, 256M, 1G.")
    return raw

def parse_cpu(raw: Optional[str]) -> Optional[str]:
    if raw is None or raw == "":
        return None
    value = raw.removesuffix("%")
    try:
        pct = float(value)
    except ValueError:
        raise HTTPException(400, "CPU quota must be a number such as 50 or 200.")
    if pct <= 0 or pct > 10000:
        raise HTTPException(400, "CPU quota must be greater than 0 and no more than 10000 percent.")
    return str(pct)

class LaunchRequest(BaseModel):
    name: str = Field(min_length=2, max_length=48, pattern=r"^[a-zA-Z0-9][a-zA-Z0-9_.-]*$")
    rootfs: str = Field(min_length=1, max_length=512)
    command: str = Field(min_length=1, max_length=256)
    memory: Optional[str] = None
    cpu: Optional[str] = None
    hostname: str = Field(default="boxforge", min_length=1, max_length=64, pattern=r"^[a-zA-Z0-9][a-zA-Z0-9.-]*$")
    userns: bool = False

    @field_validator("command")
    @classmethod
    def command_is_path(cls, value: str) -> str:
        if not value.startswith("/") or any(ch.isspace() for ch in value) or "\x00" in value:
            raise ValueError("Command must be an absolute executable path such as /bin/sh.")
        return value

def cgroup_for_pid(pid: int) -> Optional[Path]:
    try:
        data = Path(f"/proc/{pid}/cgroup").read_text()
        for line in data.splitlines():
            if line.startswith("0::"):
                relative = line.split("::", 1)[1].lstrip("/")
                path = (CGROUP_ROOT / relative).resolve()
                if path == CGROUP_ROOT or CGROUP_ROOT in path.parents:
                    return path
    except (OSError, ValueError):
        return None
    return None

def host_child_pid(parent_pid: int) -> Optional[int]:
    try:
        text = Path(f"/proc/{parent_pid}/task/{parent_pid}/children").read_text().strip()
        values = [int(v) for v in text.split()]
        return values[0] if values else None
    except (OSError, ValueError):
        return None

def drain_output(container_id: str, pipe) -> None:
    try:
        for raw in iter(pipe.readline, b""):
            line = raw.decode("utf-8", errors="replace").rstrip()
            with _lock:
                item = _containers.get(container_id)
                if item is not None:
                    item["logs"].append(line)
                    if len(item["logs"]) > 500:
                        del item["logs"][:-500]
    finally:
        try:
            pipe.close()
        except OSError:
            pass

def watch_process(container_id: str, proc: subprocess.Popen) -> None:
    proc.wait()
    with _lock:
        item = _containers.get(container_id)
        if not item:
            return
        item["status"] = "exited" if proc.returncode == 0 else "failed"
        item["exit_code"] = proc.returncode
        item["finished_at"] = now()
        name = item["name"]
    event(f"Container exited with code {proc.returncode}", name, "success" if proc.returncode == 0 else "error")

def discover_pid(item: dict) -> Optional[int]:
    proc = item.get("process")
    if not proc:
        return None
    child = host_child_pid(proc.pid)
    if child:
        return child
    return item.get("pid")

def read_int(path: Path) -> Optional[int]:
    try:
        return int(path.read_text().strip())
    except (OSError, ValueError):
        return None

def runtime_health() -> dict:
    cgroup_v2 = (CGROUP_ROOT / "cgroup.controllers").exists()
    binary_exists = BOXFORGE_BIN.is_file() and os.access(BOXFORGE_BIN, os.X_OK)
    rootfs_exists = DEFAULT_ROOTFS.is_dir()
    version = "unknown"
    if binary_exists:
        try:
            run = subprocess.run([str(BOXFORGE_BIN), "--version"], capture_output=True, text=True, timeout=2, check=False)
            version = next((line.strip() for line in run.stdout.splitlines() if line.strip()), "BoxForge")
        except (OSError, subprocess.TimeoutExpired):
            version = "BoxForge"
    ready = binary_exists and cgroup_v2 and os.geteuid() == 0
    return {"runtime": "ready" if ready else "degraded" if binary_exists else "offline",
            "binary_exists": binary_exists, "binary_path": str(BOXFORGE_BIN),
            "rootfs_exists": rootfs_exists, "default_rootfs": str(DEFAULT_ROOTFS),
            "cgroup_v2": cgroup_v2, "userns_available": Path("/proc/sys/user/max_user_namespaces").exists(),
            "version": version, "privileged": os.geteuid() == 0,
            "api_version": app.version}

@app.get("/api/health")
def health():
    return runtime_health()

@app.get("/api/containers")
def list_containers():
    with _lock:
        rows = []
        for item in _containers.values():
            proc = item.get("process")
            if proc and proc.poll() is not None and item["status"] == "running":
                item["status"] = "exited" if proc.returncode == 0 else "failed"
                item["exit_code"] = proc.returncode
            rows.append({k: v for k, v in item.items() if k not in {"process", "logs", "history", "cgroup_path"}})
        rows.sort(key=lambda x: x.get("created_at", ""), reverse=True)
    return {"containers": rows}

@app.get("/api/events")
def list_events(limit: int = Query(default=25, ge=1, le=100)):
    with _lock:
        return {"events": list(_events)[:limit]}

@app.post("/api/containers")
def create_container(payload: LaunchRequest):
    health_data = runtime_health()
    if not health_data["binary_exists"]:
        raise HTTPException(503, f"BoxForge binary not found at {BOXFORGE_BIN}. Run make in the repository root.")
    if os.geteuid() != 0:
        raise HTTPException(403, "The local API must run with privileges to create namespaces and cgroups. Start it with sudo.")
    rootfs = safe_rootfs(payload.rootfs)
    memory = parse_memory(payload.memory)
    cpu = parse_cpu(payload.cpu)
    cid = uuid.uuid4().hex[:12]
    args = [str(BOXFORGE_BIN), "run", "--hostname", payload.hostname]
    if memory:
        args += ["--memory", memory]
    if cpu:
        args += ["--cpu", cpu]
    if payload.userns:
        args.append("--userns")
    args += [str(rootfs), payload.command]
    try:
        proc = subprocess.Popen(args, cwd=str(REPO), stdin=subprocess.DEVNULL,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                start_new_session=True, close_fds=True, bufsize=0)
    except OSError as exc:
        raise HTTPException(500, f"Unable to start BoxForge: {exc}") from exc
    item = {"id": cid, "name": payload.name, "status": "running", "image": rootfs.name,
            "rootfs": str(rootfs), "command": payload.command, "hostname": payload.hostname,
            "created_at": now(), "pid": None, "exit_code": None, "memory_limit": memory,
            "cpu_limit": cpu, "userns": payload.userns, "namespaces": ["PID", "MOUNT", "UTS", "IPC", "NET"],
            "process": proc, "logs": [], "history": deque(maxlen=40), "cgroup_path": None}
    with _lock:
        _containers[cid] = item
    threading.Thread(target=drain_output, args=(cid, proc.stdout), daemon=True).start()
    threading.Thread(target=watch_process, args=(cid, proc), daemon=True).start()
    event("Container launch requested", payload.name, "success")
    time.sleep(0.12)
    if proc.poll() is not None:
        with _lock:
            item["status"] = "exited" if proc.returncode == 0 else "failed"
            item["exit_code"] = proc.returncode
    return {"container": {k: v for k, v in item.items() if k not in {"process", "logs", "history", "cgroup_path"}}}

def find_item(container_id: str) -> dict:
    with _lock:
        item = _containers.get(container_id)
    if not item:
        raise HTTPException(404, "Container not found in this dashboard session.")
    return item

@app.post("/api/containers/{container_id}/stop")
def stop_container(container_id: str):
    item = find_item(container_id)
    proc = item.get("process")
    if not proc or proc.poll() is not None:
        return {"ok": True, "status": item.get("status", "exited")}
    try:
        os.killpg(proc.pid, signal.SIGTERM)
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    with _lock:
        item["status"] = "stopped"
        item["finished_at"] = now()
    event("Stop requested", item["name"], "success")
    return {"ok": True, "status": "stopped"}

@app.get("/api/containers/{container_id}/logs")
def container_logs(container_id: str):
    item = find_item(container_id)
    with _lock:
        return {"lines": list(item["logs"])}

@app.get("/api/containers/{container_id}/metrics")
def container_metrics(container_id: str):
    item = find_item(container_id)
    pid = discover_pid(item)
    if pid and Path(f"/proc/{pid}").exists():
        item["pid"] = pid
    cg = cgroup_for_pid(pid) if pid else None
    if cg and cg.exists():
        item["cgroup_path"] = str(cg)
    cg = Path(item["cgroup_path"]) if item.get("cgroup_path") else None
    memory = read_int(cg / "memory.current") if cg else None
    limit = read_int(cg / "memory.max") if cg else None
    if limit == (1 << 63) - 1 or limit == -1:
        limit = None
    cpu_percent = 0.0
    if cg:
        stat_path = cg / "cpu.stat"
        try:
            stat_values = dict(line.split() for line in stat_path.read_text().splitlines() if len(line.split()) == 2)
            usage = int(stat_values.get("usage_usec", "0"))
            prior = item.get("_cpu_sample")
            stamp = time.monotonic()
            if prior:
                delta_time = max(0.001, stamp - prior[0])
                cpu_percent = max(0.0, (usage - prior[1]) / 1000000 / delta_time * 100)
            item["_cpu_sample"] = (stamp, usage)
        except (OSError, ValueError):
            pass
    hist = item["history"]
    hist.append({"time": datetime.now().strftime("%H:%M:%S"), "cpu": round(min(cpu_percent, 1000), 1),
                 "memory": round((memory / limit) * 100, 1) if memory and limit else 0})
    return {"pid": pid, "cpu_percent": round(cpu_percent, 1), "memory_bytes": memory,
            "memory_limit_bytes": limit, "cgroup_path": str(cg) if cg else None, "history": list(hist)}

@app.get("/api/status")
def status():
    return {"health": runtime_health(), "containers": list_containers()["containers"]}
