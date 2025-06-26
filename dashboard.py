import matplotlib
matplotlib.use("TkAgg")  # Forcer le backend compatible avec Tkinter

import tkinter as tk
from tkinter import ttk, messagebox
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import socket
import time
import os
from datetime import datetime
import re


class StylishIoTDashboard:
    #init function 
    def __init__(self, root):
        self.root = root
        self.root.title("Smart House")
        self.root.geometry("1280x800")
        self.root.configure(bg="#f0f4f8")

        self.server_ip = "192.168.214.147" #change the ip adresse
        self.server_port = 12345

        self.temperature_data = []
        self.humidity_data = []
        self.time_data = []

        self.setup_styles()
        self.create_layout()
        self.load_history_from_log()
#style function 
    def setup_styles(self):
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("Card.TFrame", background="#ffffff", relief="raised", borderwidth=1)
        style.configure("TLabel", background="#ffffff", foreground="#2d3748", font=("Sans", 11 , "bold"))
        style.configure("Title.TLabel", background="#2b6cb0", font=("Sans", 20, "bold"), foreground="white")
        style.configure("TButton", font=("Sans", 10, "bold"), padding=6)
        style.configure("Bold.TLabelframe.Label", font=("Sans", 12, "bold"))


    def create_layout(self):
        header = tk.Frame(self.root, bg="#2b6cb0", height=60)
        header.pack(fill=tk.X)
        title = ttk.Label(header, text="Smart House", style="Title.TLabel")
        title.pack(side=tk.LEFT, padx=20, pady=10)

        top_right = tk.Frame(header, bg="#2b6cb0")
        top_right.pack(side=tk.RIGHT)
        ttk.Button(top_right, text="Quitter Serveur", command=self.quit_server).pack(side=tk.LEFT, padx=5, pady=10)
        ttk.Button(top_right, text="Quitter", command=self.quit_client).pack(side=tk.LEFT, padx=5, pady=10)

        cards_frame = tk.Frame(self.root, bg="#f0f4f8")
        cards_frame.pack(fill=tk.X, padx=20)

        def create_card(parent, text):
            card = ttk.Frame(parent, style="Card.TFrame")
            card.pack(side=tk.LEFT, expand=True, fill=tk.BOTH, padx=10, pady=10)
            ttk.Label(card, text=text).pack(pady=(10, 0))
            value = ttk.Label(card, text="--", font=("Sans", 16, "bold"), foreground="#2b6cb0")
            value.pack(pady=(5, 10))
            return value

        self.temp_label = create_card(cards_frame, "Température")
        self.hum_label = create_card(cards_frame, "Humidité")
        self.led_label = create_card(cards_frame, "LED")
        self.dist_label = create_card(cards_frame, "Distance")
        self.servo_label = create_card(cards_frame, "Servomoteur")
        control_card = ttk.Frame(self.root, style="Card.TFrame")
        control_card.pack(fill=tk.X, padx=20, pady=5)
        controls = tk.Frame(control_card, bg="#ffffff")
        controls.pack(pady=15)

        ttk.Button(controls, text="Rafraîchir", command=self.update_data).grid(row=0, column=0, padx=10, pady=10)
        ttk.Button(controls, text="Distance", command=self.update_distance).grid(row=0, column=1, padx=10, pady=10)
        ttk.Button(controls, text="Inverser LED", command=self.turn_off_led).grid(row=0, column=2, padx=10, pady=10)
        ttk.Button(controls, text="Ouvrir Porte", command=self.open_door).grid(row=0, column=3, padx=10, pady=10)
        ttk.Button(controls, text="Historique", command=self.afficher_historique).grid(row=0, column=4, padx=10, pady=10)

        ttk.Label(controls, text="Intervalle (sec):").grid(row=0, column=5, padx=10)
        self.selected_interval = tk.StringVar(value="60")
        interval_menu = ttk.OptionMenu(controls, self.selected_interval, "60", "60", "120", "180", command=self.change_interval)
        interval_menu.grid(row=0, column=6, padx=10)
        self.interval_feedback = ttk.Label(controls, text="", foreground="green", background="#ffffff")
        self.interval_feedback.grid(row=0, column=7, padx=10)

        graph_card = ttk.LabelFrame(self.root, text="Température / Humidité", style="Bold.TLabelframe")
        graph_card.pack(fill=tk.BOTH, expand=True, padx=20, pady=10)

        self.fig = Figure(figsize=(9.5, 4), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.canvas = FigureCanvasTkAgg(self.fig, master=graph_card)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    def send_command(self, command):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.connect((self.server_ip, self.server_port))
                sock.sendall(command.encode())
                response = sock.recv(1024).decode().strip()
            return response
        except Exception as e:
            return f"Erreur: {e}"

    def update_data(self):
        temp = self.send_command("TEMP")
        hum = self.send_command("Hum")
        try:
            temp_val = float(temp)
            hum_val = float(hum)
            current_time = time.strftime("%H:%M:%S")
            self.temperature_data.append(temp_val)
            self.humidity_data.append(hum_val)
            self.time_data.append(current_time)
            if len(self.time_data) > 10:
                self.temperature_data.pop(0)
                self.humidity_data.pop(0)
                self.time_data.pop(0)
            self.temp_label.config(text=f"{temp_val:.1f} °C")
            self.hum_label.config(text=f"{hum_val:.1f} %")
            self.save_to_log(temp_val, hum_val)
        except ValueError:
            self.temp_label.config(text="Erreur")
            self.hum_label.config(text="Erreur")
            
#Function to save the temp and hum historique
    def save_to_log(self, temp, hum):
        try:
            with open("copiedht.log", "a") as f:
                f.write(f"{temp},{hum}\n")
        except:
            pass

    def load_history_from_log(self):
        self.temperature_data.clear()
        self.humidity_data.clear()
        self.time_data.clear()
        if os.path.exists("copiedht.log"):
            try:
                with open("copiedht.log", "r") as f:
                    lines = f.readlines()
                    for i, line in enumerate(lines[-10:]):
                        parts = line.strip().split(",")
                        if len(parts) == 2:
                            self.temperature_data.append(float(parts[0]))
                            self.humidity_data.append(float(parts[1]))
                            self.time_data.append(f"T-{10-i}")
                    if self.temperature_data and self.humidity_data:
                        self.temp_label.config(text=f"{self.temperature_data[-1]:.1f} °C")
                        self.hum_label.config(text=f"{self.humidity_data[-1]:.1f} %")
                        self.plot_graph()
            except:
                pass

# Function to update the distance
    def update_distance(self):
        dist = self.send_command("Dist")
        self.dist_label.config(text=f"{dist} cm")
# function to turn off the led
    def turn_off_led(self):
        result = self.send_command("LED")
        self.led_label.config(text=result)
#function to turn off the servomotors
    def open_door(self):
        result = self.send_command("Servo")
        self.servo_label.config(text=f"{result}°") 
#function to change the time intervalle for saving the temp and hum
    def change_interval(self, value):
        res = self.send_command(f"SET_INTERVAL {value}")
        self.interval_feedback.config(text=f"{res}")
#function to quit the server
    def quit_server(self):
        confirm = messagebox.askyesno("Confirmation", "Voulez-vous vraiment arrêter le serveur ?")
        if confirm:
            response = self.send_command("Serveur")
            messagebox.showinfo("Serveur", f"Réponse: {response}")
#function to quit the client
    def quit_client(self):
        self.root.quit()

    def plot_graph(self):
        self.ax.clear()
        self.ax.plot(self.time_data, self.temperature_data, label="Température (°C)", marker='o', color='tomato')
        self.ax.plot(self.time_data, self.humidity_data, label="Humidité (%)", marker='x', color='dodgerblue')
        self.ax.set_title("Évolution des données", fontsize=14, fontweight='bold', color='blue')
        self.ax.set_xlabel("Heure")
        self.ax.set_ylabel("Valeur")
        self.ax.set_ylim(0, 100)
        self.ax.legend()
        self.ax.grid(True)
        self.fig.tight_layout()
        self.canvas.draw()


    def afficher_historique(self):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                sock.connect((self.server_ip, self.server_port))
                sock.sendall(b"Histo")

                with open("copiedht.log", "w") as copiedht:
                    while True:
                        data = sock.recv(1024).decode()
                        if data.strip() == "__END__":
                            break
                        copiedht.write(data)

            with open("copiedht.log", "r") as f:
                lines = f.readlines()
                lines = lines[:-2] if len(lines) > 2 else lines

                self.temperature_data.clear()
                self.humidity_data.clear()
                self.time_data.clear()

                for line in lines:
                    match = re.search(r"\[(.*?)\] : Temp : (\d+) , Hum : (\d+)", line)
                    if match:
                        time_str = match.group(1)
                        temp = float(match.group(2))
                        hum = float(match.group(3))
                        heure = datetime.strptime(time_str, "%a %b %d %H:%M:%S %Y").strftime("%H:%M:%S")
                        self.temperature_data.append(temp)
                        self.humidity_data.append(hum)
                        self.time_data.append(heure)

            self.plot_graph()

        except Exception as e:
            messagebox.showerror("Erreur", f"Erreur chargement historique: {e}")

def run_stylish_dashboard():
    root = tk.Tk()
    app = StylishIoTDashboard(root)
    root.mainloop()

run_stylish_dashboard()