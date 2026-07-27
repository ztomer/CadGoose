# Command Socket Protocol

CadGoose communicates with its running instance via a Unix domain socket. The socket is created on startup and exists for the lifetime of the process.

## Architecture

```
CLI process                         Running CadGoose instance
     |                                       |
     |----------- connect -------------------->|
     |----------- "arg0\targ1\n" --------->|
     |<---------- "response\n" --------------|
     |----------- close -------------------->|
```

- **Server**: Background thread started on app launch. Listens with `backlog=8`.
- **Client**: Any CLI invocation with a control command connects, sends, reads, closes.

## Message Format

**Request**: tab-separated arguments, terminated by newline
```
command\targ1\targ2\n
```

**Response**: newline-terminated plain text, or empty string on error.

Escape `\t`, `\n`, and `\` in fields with backslash: `\\`, `\t`, `\n`.

## Socket Path

| Platform | Path |
|---|---|
| macOS | `/tmp/desktop-goose.sock` |
| Linux | `$XDG_RUNTIME_DIR/desktop-goose.sock` (fallback: `/tmp/desktop-goose-<uid>.sock`) |

## Commands

All commands are sent as the first argument. The handler is in `AppActions_HandleCommand()` (`src/common/app_actions.cpp`).

### `start`

Daemonizes a new background process. Fails if already running.

```bash
./build/CadGoose start
# => Desktop Goose started in background
```

No socket round-trip — runs `fork()` + `setsid()` directly. Use `status` to check if already running.

---

### `spawn [name]`

Spawns a new goose. If `name` is given, uses it; otherwise generates `"Goose N"`.

```bash
./build/CadGoose spawn Pip
# => ok id=1

./build/CadGoose spawn
# => ok id=2
```

| Arg | Type | Description |
|---|---|---|
| `name` | optional | Name for the new goose |

---

### `clear`

Removes all geese and dropped items. Resets goose IDs and cursor grabber.

```bash
./build/CadGoose clear
# => ok
```

---

### `status`

Returns server status, goose count, config path, memory usage (Linux), and all registered config key-value pairs.

```bash
./build/CadGoose status
# => running=1
#    goose_count=2
#    config_path=/Users/ztomer/Projects/CadGoose/cadgoose.toml
#    general.globalScale=1.0
#    ...
```

---

### `ram`

Returns memory usage from `/proc/self/status`. Linux only; returns empty string on macOS.

```bash
./build/CadGoose ram
# => ram_rss_mb=85.32
#    ram_peak_mb=102.45
#    ram_virtual_mb=412.10
```

---

### `quit`

Clears all geese, drops items, and quits the application.

```bash
./build/CadGoose quit
# => ok cleared and quitting
```

---

### `fetch [meme|text]`

Forces the first goose to fetch an item. Type `0` (meme) is default if not specified.

```bash
./build/CadGoose fetch meme
# => ok force_fetch type=0

./build/CadGoose fetch text
# => ok force_fetch type=1

./build/CadGoose fetch
# => ok force_fetch type=0  (default)
```

| Arg | Type | Description |
|---|---|---|
| `meme` | optional | Fetch a random meme image |
| `text` | optional | Fetch a random text note |

The goose enters `FETCHING` state, walks to the fetch target, picks up the item, enters `RETURNING` state, walks to the drop target, and drops the item.

---

### `spawn` (detailed)

Spawns a goose at a random position with a wander target. The goose begins running behavior immediately.

```bash
# Named goose
./build/CadGoose spawn Boss
# => ok id=0

# Auto-generated name "Goose 1"
./build/CadGoose spawn
# => ok id=1
```

---

---

### `spawn_baby_stalin` / `spawn_stalin`

Spawns a Stalin-mode goose variant that plays Gulag audio instead of honking.

```bash
./build/CadGoose spawn_baby_stalin
# => ok id=3
```

BabyStalin has `m_canHonk = true` and overrides `onHonk()` to play Gulag.

---

### `clear_dropped`

Removes all dropped items from the screen without affecting geese. Useful for test isolation.

```bash
./build/CadGoose clear_dropped
# => ok
```

---

## Error Responses

| Response | Meaning |
|---|---|
| `error missing command\n` | Empty request |
| `error no goose\n` | `fetch` called with no geese |
| `error unknown command: <name>\n` | Unrecognized first argument |
| `(connection refused)` | Server not running — client prints `Desktop Goose is not running` |

## Programmatic Access

### Python

```python
import socket

SOCK = "/tmp/desktop-goose.sock"  # macOS
# SOCK = "/tmp/desktop-goose-1000.sock"  # Linux (with uid)

def send_command(*args):
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(SOCK)
    sock.sendall(("\t".join(args) + "\n").encode())
    resp = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            break
        resp += chunk
    sock.close()
    return resp.decode()

# Check if running
print(send_command("status"))

# Spawn a goose
print(send_command("spawn", "Nelly"))

# Trigger a fetch
print(send_command("fetch", "meme"))

# Check updated state
print(send_command("status"))
```

### Rust

```rust
use std::os::unix::net::UnixStream;
use std::io::{Read, Write};

fn send_command(sock_path: &str, args: &[&str]) -> String {
    let mut sock = UnixStream::connect(sock_path).unwrap();
    let msg = args.join("\t") + "\n";
    sock.write_all(msg.as_bytes()).unwrap();
    let mut resp = String::new();
    sock.read_to_string(&mut resp).unwrap();
    resp
}

fn main() {
    let resp = send_command("/tmp/desktop-goose.sock", &["status"]);
    println!("{}", resp);
}
```

### Bash (netcat)

```bash
echo -e "status" | nc -U /tmp/desktop-goose.sock
echo -e "fetch\tmeme" | nc -U /tmp/desktop-goose.sock
```

## Implementation

| File | Role |
|---|---|
| `include/command_socket.h` | Public API |
| `src/platform/macos/command_socket.mm` | Server/client (dispatch_async to main thread) |
| `src/platform/linux/command_socket.cpp` | Server/client (GLib integration) |
| `src/common/app_actions.cpp:195` | Command handler — `AppActions_HandleCommand()` |
| `src/common/app_cli.cpp:22` | CLI control-command list — `IsControlCommand()` |