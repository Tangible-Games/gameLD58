import http.server
import socketserver
import subprocess
import threading
import sys
import os
import platform
import time
import termios
import tty

PORT = 9000
Handler = http.server.SimpleHTTPRequestHandler

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True

httpd = None
def run_server():
    global httpd
    httpd = ThreadedTCPServer(("", PORT), Handler)
    print(f"Serving HTTP on port {PORT} (http://localhost:{PORT}/)")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass  # allow Ctrl‑C to break out if you run the script manually

server_thread = threading.Thread(target=run_server, daemon=True)
server_thread.start()

def open_browser(url):
    if platform.system() == "Darwin":          # macOS
        subprocess.run(["open", url])
    elif os.name == "posix":                   # Linux / macOS
        subprocess.run(["xdg-open", url])
    else:                                      # Windows
        subprocess.run(["cmd", "/c", "start", url], shell=True)

open_browser(f"http://localhost:{PORT}/index.html")

def wait_for_esc():
    print("\nPress Esc to stop the server…")
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)              # switch to raw mode
        while True:
            ch = os.read(fd, 1)
            if ch == b'\x1b':       # ESC
                break
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

wait_for_esc()

print("\nStopping the server…")
if httpd:
    httpd.shutdown()   # this causes serve_forever() to exit
server_thread.join()
print("Server stopped.")