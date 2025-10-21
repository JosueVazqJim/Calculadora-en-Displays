from calculadora import Calculadora
from contador import Contador
from widgetsContador import WidgetsContador
from httpClient import HTTPClient
import tkinter as tk

class InterfazPrincipal:
	def __init__(self):
		self.ventana = tk.Tk()
		self.ventana.geometry("1220x700")
		self.ventana.title("CalculadoraOGT")
		self.ventana.config(bg="black")
		self.titulo = tk.Label(self.ventana, text="Controlador del Display", font=("Courier", 30, "bold"), bg="black", fg="white")
		self.titulo.pack(pady=20)
		 # Crear e insertar los widgets	
		self.httpClient = HTTPClient("192.168.100.99")
		self.contador = Contador(self.ventana)
		self.contador.insertar(500, 200)
		self.widgets_contador = WidgetsContador(self.ventana, self.contador.display, self.httpClient)
		self.widgets_contador.insertar(800, 100)
		self.calculadora = Calculadora(self.ventana, self.contador.display, self.widgets_contador, self.httpClient)
		self.calculadora.insertar(40, 100)

	def ejecutar(self):
		self.ventana.mainloop()

if __name__ == "__main__":
	interfaz = InterfazPrincipal()
	interfaz.ejecutar()
