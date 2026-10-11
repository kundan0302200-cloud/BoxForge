# BoxForge Studio

A dark, responsive local control plane for the existing BoxForge Linux container runtime.

## Start locally

From the repository root, compile the runtime first:

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

Visit http://127.0.0.1:5173.

The API must have the privileges required by BoxForge. Keep it bound to loopback; do not expose it to a public network. The UI manages non-interactive workloads; use the CLI for an interactive shell.
