import sys
import socket
import tkinter as tk
from tkinter import messagebox


class Requestor:
    def __init__(self):
        self.header_delimiter = '\n'
        if len(sys.argv) >= 3:
            self.server_config = (socket.gethostbyname(
                sys.argv[1]), int(sys.argv[2]))
        else:
            self.server_config = ("127.0.0.1", 1235)

    def request(self, body: str) -> str:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(self.server_config)

        message = f"{len(body)}{self.header_delimiter}{body}"

        sock.sendall(message.encode("utf-8"))

        response = sock.recv(1).decode("utf-8")
        while response[-1] != self.header_delimiter:
            response += sock.recv(1).decode("utf-8")

        response_body_size = int(response)
        received_size = 0

        buffer = []

        while received_size < response_body_size:
            b = sock.recv(min(2, response_body_size - received_size))
            received_size += len(b)
            buffer.append(b.decode("utf-8"))

        sock.close()
        return ''.join(buffer)


class Client:
    def update_scripts_list(self):
        scripts_list = self.requestor.request(
            self.get_scripts_command).splitlines()

        self.listbox.delete(0, tk.END)

        for script in scripts_list:
            self.listbox.insert(tk.END, script)

    def run_script(self):
        select_list = self.listbox.curselection()
        selected_script = self.listbox.get(select_list[0])
        args = self.arguments.get()
        result = self.requestor.request(
            f"{self.exec_script_command} {selected_script} {args}")

        win = tk.Toplevel()
        win.geometry("250x250")
        win.title("Script result")
        tk.Label(
            win, text=f"Result of executing {selected_script} with arguments: {args}", wraplength=250, justify="center").pack()
        tk.Label(win, text=result, wraplength=250, justify="center").pack()

    def __init__(self, requestor: Requestor):
        self.get_scripts_command = "GET_SCRIPTS"
        self.exec_script_command = "EXEC"
        self.requestor = requestor

        self.root = tk.Tk()
        self.root.title("Script Client")
        self.root.geometry('500x400')

        self.arguments = tk.StringVar()

        self.label = tk.Label(self.root, text="\nScripts:")
        self.label.pack()

        self.listbox = tk.Listbox(self.root, width=40)
        self.listbox.pack()

        self.refresh_button = tk.Button(
            self.root, text="Refresh scripts", command=self.update_scripts_list)
        self.refresh_button.pack()

        self.label = tk.Label(self.root, text="\nArguments: ")
        self.label.pack()

        self.arguments_entry = tk.Entry(
            self.root, width=50, textvariable=self.arguments)
        self.arguments_entry.pack()

        self.run_button = tk.Button(
            self.root, text="Run", command=self.run_script)
        self.run_button.pack()

        self.msg = tk.Message(self.root, text='')
        self.msg.pack()

        try:
            self.update_scripts_list()
        except:
            messagebox.showerror("Error", "Could not connect to server")
            self.root.destroy()
            exit()

    def start(self):
        self.root.mainloop()


if __name__ == "__main__":
    client = Client(Requestor())
    client.start()
