# BoxForge Studio API

A local FastAPI control plane for the BoxForge runtime. The UI calls this API; it does not simulate runtime state.

## Requirements

- Linux (Ubuntu/WSL2 recommended)
- Python 3.10+
- BoxForge compiled in the repository root (`make`)
- cgroups v2 and namespace support
- Privileges needed by the existing BoxForge runtime

## Start

From the repository root:

```bash
make
python3 -m venv .venv
. .venv/bin/activate
pip install -r dashboard/requirements.txt
sudo -E .venv/bin/uvicorn dashboard.api.server:app --host 127.0.0.1 --port 8000
```

In a second terminal:

```bash
cd dashboard
npm install
npm run dev
```

Open http://127.0.0.1:5173.

If sudo does not preserve the virtual environment path, run the API with the absolute path to `.venv/bin/uvicorn`. Do not bind this API to 0.0.0.0.

## Runtime behavior and limitations

- The API launches the existing `boxforge` executable; it does not replace the C runtime.
- Container records and event history are in memory and reset when the API restarts.
- Commands are passed as an argument array, not through a shell. The dashboard currently targets non-interactive workloads; use the BoxForge CLI for an interactive shell.
- Rootfs directories must be under the repository or under an explicitly configured `BOXFORGE_ALLOWED_ROOTS` directory.
- CPU and memory metrics are read from the container's cgroup when the host process can be associated with it. Missing kernel counters are shown as unavailable/zero, not fabricated.
- The UI is a local development control plane, not a production orchestrator or security boundary.

Optional environment variables:

```bash
BOXFORGE_BIN=/absolute/path/to/boxforge
BOXFORGE_ROOTFS=/absolute/path/to/rootfs
BOXFORGE_ALLOWED_ROOTS=/path/one:/path/two
```

API docs are available at http://127.0.0.1:8000/api/docs while the service is running.
