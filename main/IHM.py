import tkinter as tk
from tkinter import messagebox
import serial
import serial.tools.list_ports

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
    if data in {'10', '45', '60', '80'}:
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
        messagebox.showwarning("Warning", "Por favor, insira um valor válido (10, 45, 60, 80).")

def update_angle_label(angle):
    """Atualiza o rótulo de ângulo atual com o valor recebido."""
    angle_value = int(angle)
    if angle_value in {10, 45, 60, 80}:
        angle_label.config(text=f"Ângulo Atual: {angle_value}°")
    else:
        angle_label.config(text="Ângulo Atual: N/A")

'''
def turn_off():
    """Envia um comando para desligar o dispositivo."""
    try:
        angle_zero = '0'
        ser.write(angle_zero.encode())  # Converte string para bytes e envia
        angle_label.config(text=f"Ângulo Atual: {int(angle_zero)}°")
        response = ser.readline().decode('utf-8').strip() # Aguarda uma resposta do ESP32
        if response:
            print(f"Resposta recebida: {response}")  # Para depuração
        else:
            messagebox.showinfo("Info", f"Mensagem enviada: {angle_zero}\nSem resposta do ESP32")
        entry.delete(0, tk.END)  # Limpa o campo de entrada
    except serial.SerialException as e:
        messagebox.showerror("Erro", f"Erro ao enviar dados: {e}")
    
        
    except serial.SerialException as e:
        messagebox.showerror("Erro", f"Erro ao enviar comando OFF: {e}")
'''

def exit_app():
    """Fecha a aplicação e a porta serial."""
    try:
        ser.close()  # Fecha a porta serial
    except serial.SerialException as e:
        print(f"Erro ao fechar a porta serial: {e}")
    root.destroy()

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
'''
off_button = tk.Button(root, text="OFF", command=turn_off, bg='red', fg='white')
off_button.pack(pady=5)
'''

# Cria um botão para sair da aplicação
exit_button = tk.Button(root, text="EXIT", command=exit_app, bg='white', fg='black')
exit_button.pack(pady=10)

# Cria um rótulo para exibir o ângulo atual
angle_label = tk.Label(root, text="Ângulo Atual: N/A", font=("Arial", 12), bg='white', fg='green')
angle_label.pack(side='top', anchor='ne', padx=10, pady=10)

# Associa a tecla Enter ao botão de enviar
root.bind('<Return>', lambda event: validate_and_send_data())

# Inicia o loop da interface gráfica
root.mainloop()