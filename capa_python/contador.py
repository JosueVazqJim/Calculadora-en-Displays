import tkinter as tk


#solo el Contador, sera incrustado en la interfaz principal
class Contador:
	def __init__(self, master):
		self.master = master  # es el tk principal
		self.frame = tk.Frame(master, bg="black")
		self.titulo = tk.Label(self.frame, text="Display", font=("Courier", 25, "bold"), bg="black", fg="white")
		self.titulo.grid(row=0, column=0, columnspan=4, pady=20)
		self.display = tk.Label(self.frame, text="00", font=("OCR A Extended", 135), bg="black", fg="white")
		self.display.grid(row=1, column=0, columnspan=4, pady=20)

	def insertar(self, x, y):
		self.frame.pack()
		self.frame.place(x=x, y=y)