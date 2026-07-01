#!/usr/bin/env python3
"""Write Claude Code hook status for Vico Keyboard.

This script is intentionally dependency-free. Claude Code passes hook event JSON
on stdin; for the MVP we only need the event name plus the state/message passed
from settings.json.
"""

from __future__ import annotations

import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path


DEFAULT_STATE = "READY"
DEFAULT_MESSAGE = "Claude Code"


def read_stdin_json() -> dict:
    raw = sys.stdin.read().strip()
    if not raw:
        return {}
    try:
        return json.loads(raw)
    except json.JSONDecodeError as exc:
        print(f"vico-status: ignored invalid hook JSON: {exc}", file=sys.stderr)
        return {}


def find_project_dir() -> Path:
    env_dir = os.environ.get("CLAUDE_PROJECT_DIR")
    if env_dir:
        return Path(env_dir)
    return Path(__file__).resolve().parents[2]


def main() -> int:
    state = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_STATE
    message = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_MESSAGE
    event = read_stdin_json()

    project_dir = find_project_dir()
    status_dir = project_dir / ".vico"
    status_dir.mkdir(parents=True, exist_ok=True)
    status_file = status_dir / "status.json"

    payload = {
        "state": state.upper(),
        "message": message,
        "source": "claude-code",
        "hook_event_name": event.get("hook_event_name"),
        "cwd": event.get("cwd"),
        "updated_at": datetime.now(timezone.utc).isoformat(),
    }

    tmp_file = status_file.with_suffix(".json.tmp")
    tmp_file.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    tmp_file.replace(status_file)

    # Keep stdout quiet unless another hook expects JSON from us.
    print(f"vico-status: {payload['state']} - {payload['message']}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
