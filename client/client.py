import socket
import tkinter as tk
from tkinter import messagebox

class ScriptClient:

    def get_server_settings(self):
        try:
            with open(self.settings_file, "r") as settings:
                ip = settings.readline().strip()
                port = int(settings.readline().strip())
                return (ip, port)
        except:
            messagebox.showerror("Error", "Could not read config file")
            self.root.destroy()


    def update_scripts_list(self):
        try:
            command = "GET_SCRIPTS"
            new_line ='\n'
            message = f"{len(command)}{new_line}{command}"
            self.client_socket.sendall(message.encode())
            tmp_data = self.client_socket.recv(1)
            message_size = ''
            while tmp_data.decode() != '\n':
                message_size += tmp_data.decode()
                tmp_data = self.client_socket.recv(1)
            scripts_list = self.client_socket.recv(int(message_size)).decode()
            self.client_socket.close()
            scripts_list = scripts_list.splitlines()
            scripts_list = '\n'.join(scripts_list[2:])
            scripts_list = scripts_list.splitlines()
            for script in scripts_list:
                self.listbox.insert(tk.END, script)
            self.client_socket.close()
        except:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect(self.server_address)
            self.update_scripts_list()
            
            
    def run_script(self):
        try:
            select_list = self.listbox.curselection()
            selected_script = self.listbox.get(select_list[0])
            command = f"EXEC {selected_script} {self.arguments.get()}"
            new_line ='\n'
            message = f"{len(command)}{new_line}{command}"
            self.client_socket.sendall(message.encode())
            tmp_data = self.client_socket.recv(1)
            message_size = ''
            while tmp_data.decode() != '\n':
                message_size += tmp_data.decode()
                tmp_data = self.client_socket.recv(1)
            result = self.client_socket.recv(int(message_size)).decode()
            self.client_socket.close()
            messagebox.showinfo("Result", result)
        except:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect(self.server_address)
            self.run_script()
        

    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Script Client")
        self.root.geometry('500x400')

        self.settings_file = "config.txt"
        self.server_address = self.get_server_settings()
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.client_socket.connect(self.server_address)
        except:
            messagebox.showerror("Error", "Could not connect to server")
            self.root.destroy()

        self.arguments = tk.StringVar()

        self.label = tk.Label(self.root, text="\nScripts list:")
        self.label.pack()

        self.listbox = tk.Listbox(self.root, width= 40)
        self.listbox.pack()

        self.refresh_button = tk.Button(self.root, text="Update scripts", command=self.update_scripts_list)
        self.refresh_button.pack()
        
        self.label = tk.Label(self.root, text="\nEnter arguments if needed: ")
        self.label.pack()
        
        self.arguments_entry = tk.Entry(self.root, width=50, textvariable=self.arguments)
        self.arguments_entry.pack()

        self.run_button = tk.Button(self.root, text="Run", command=self.run_script)
        self.run_button.pack()

    def start(self):
        self.root.mainloop()
        
if __name__ == "__main__":
    client = ScriptClient()
    client.start()

