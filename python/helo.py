import tkinter as tk
from tkinter import scrolledtext
import threading
from datetime import datetime
import websocket

class ChatApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Chat Online")
        self.root.geometry("500x600")

        self.ws = None

        # URL
        self.url_entry = tk.Entry(root)
        self.url_entry.insert(0, "ws://localhost:8000/ws")
        self.url_entry.pack(fill=tk.X, padx=10, pady=5)

        self.connect_btn = tk.Button(root, text="Kết nối", command=self.connect)
        self.connect_btn.pack()

        # Chat box
        self.chat = scrolledtext.ScrolledText(root, state=tk.DISABLED)
        self.chat.pack(expand=True, fill=tk.BOTH, padx=10, pady=5)

        # Input
        frame = tk.Frame(root)
        frame.pack(fill=tk.X)

        self.entry = tk.Entry(frame)
        self.entry.pack(side=tk.LEFT, expand=True, fill=tk.X)
        self.entry.bind("<Return>", lambda e: self.send())

        tk.Button(frame, text="Gửi", command=self.send).pack(side=tk.RIGHT)

    def log(self, msg):
        self.chat.config(state=tk.NORMAL)
        time = datetime.now().strftime("%H:%M")
        self.chat.insert(tk.END, f"[{time}] {msg}\n")
        self.chat.config(state=tk.DISABLED)
        self.chat.see(tk.END)

    def connect(self):
        url = self.url_entry.get()

        def run():
            self.ws = websocket.WebSocketApp(
                url,
                on_message=lambda ws, msg: self.root.after(0, lambda: self.log("Đối phương: " + msg))
            )
            self.ws.run_forever()

        threading.Thread(target=run, daemon=True).start()
        self.log("Đã kết nối server")

    def send(self):
        msg = self.entry.get()
        if msg and self.ws:
            self.ws.send(msg)
            self.log("Bạn: " + msg)
            self.entry.delete(0, tk.END)

root = tk.Tk()
app = ChatApp(root)
root.mainloop()