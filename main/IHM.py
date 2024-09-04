import tkinter as tk
from tkinter import messagebox
import serial
import serial.tools.list_ports
import threading

# Liste todas as portas COM disponíveis
ports = serial.tools.list_ports.comports()
available_ports = [port.device for port in ports]
print("Portas disponíveis:", available_ports)

# Configure a porta serial
PORT = 'COM4'  # Substitua pela sua porta COM se necessário
BAUDRATE = 115200
TIMEOUT = 1

# Tenta abrir a porta serial
try:
    ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
    print(f"Conectado à porta {PORT}")
except serial.SerialException as e:
    print(f"Erro ao abrir a porta {PORT}: {e}")
    exit()

def validate_and_send_data():
    """Valida a entrada para ser um dos valores permitidos e envia para o ESP32."""
    data = entry.get()  # Obtém o texto do campo de entrada
    if data in {'30', '55', '65', '70', '75', '80'}:
        try:
            ser.write(data.encode())  # Converte string para bytes e envia
            update_angle_label(data)  # Atualiza o rótulo de ângulo com o valor enviado
            # Aguarda uma resposta do ESP32
            response = ser.readline().decode('utf-8').strip()
            if response:
                print(f"Resposta recebida: {response}")  # Para depuração
            else:
                messagebox.showinfo("Info", f"Mensagem enviada: {data}\nSem resposta do ESP32")
            entry.delete(0, tk.END)  # Limpa o campo de entrada
        except serial.SerialException as e:
            messagebox.showerror("Erro", f"Erro ao enviar dados: {e}")
    else:
        messagebox.showwarning("Warning", "Por favor, insira um valor válido (30...80 em 5 em 5 graus).")

def update_angle_label(angle):
    """Atualiza o rótulo de ângulo atual com o valor recebido."""
    angle_value = int(angle)
    if angle_value in {0, 30, 55, 65, 75, 80}:
        angle_label.config(text=f"Ângulo Atual: {angle_value}°")
    else:
        angle_label.config(text="Ângulo Atual: N/A")

def turn_off():
    """Envia um comando para desligar o dispositivo."""
    try:
        ser.write(b'OFF')
        messagebox.showinfo("Info", "Comando OFF enviado")
    except serial.SerialException as e:
        messagebox.showerror("Erro", f"Erro ao enviar comando OFF: {e}")

def exit_app():
    """Fecha a aplicação e a porta serial."""
    try:
        ser.close()  # Fecha a porta serial
    except serial.SerialException as e:
        print(f"Erro ao fechar a porta serial: {e}")
    root.destroy()

def read_from_serial():
    """Função para ler valores do ESP32 e atualizar o rótulo na interface."""
    while True:
        if ser.in_waiting > 0:
            try:
                data = ser.readline().decode('utf-8').strip()
                if data.startswith("ADC:"):
                    adc_value = int(data.split(":")[1])
                    adc_label.config(text=f"Leitura ADC: {adc_value}")
                print(f"Recebido: {data}")  # Para depuração
            except Exception as e:
                print(f"Erro ao ler da serial: {e}")

# Configura a janela principal
root = tk.Tk()
root.title("FAN-PLATE")

# Define a cor de fundo da janela
root.configure(bg='black')

# Cria um rótulo com o título
label = tk.Label(root, text="FAN-PLATE", font=("Arial", 16), bg='black', fg='white')
label.pack(pady=10)

# Cria um campo de entrada de texto
entry = tk.Entry(root, width=40)
entry.pack(pady=10)

# Cria um botão para enviar dados
send_button = tk.Button(root, text="Enviar", command=validate_and_send_data, bg='green', fg='white')
send_button.pack(pady=10)

# Cria um botão para desligar o dispositivo
off_button = tk.Button(root, text="OFF", command=turn_off, bg='red', fg='white')
off_button.pack(pady=5)

# Cria um botão para sair da aplicação
exit_button = tk.Button(root, text="EXIT", command=exit_app, bg='white', fg='black')
exit_button.pack(pady=10)

# Cria um rótulo para exibir o ângulo atual
angle_label = tk.Label(root, text="Ângulo Atual: N/A", font=("Arial", 12), bg='white', fg='green')
angle_label.pack(side='top', anchor='ne', padx=10, pady=10)

# Cria um rótulo para exibir o valor ADC
adc_label = tk.Label(root, text="Leitura ADC: N/A", font=("Arial", 12), bg='white', fg='blue')
adc_label.pack(side='top', anchor='ne', padx=10, pady=5)

# Associa a tecla Enter ao botão de enviar
root.bind('<Return>', lambda event: validate_and_send_data())

# Inicia uma thread para ler a porta serial
serial_thread = threading.Thread(target=read_from_serial)
serial_thread.daemon = True
serial_thread.start()

# Inicia o loop da interface gráfica
root.mainloop()
