import tkinter as tk


#solo widgets de calculadora, sera incrustado en la interfaz principal
class Calculadora:
	def __init__(self, master, display, widgets, httpClient):
		self.master = master  # es el tk principal
		self.frame = tk.Frame(master, bg="gray")
		self.entrada = tk.Entry(self.frame, width=16, font=("Arial", 30))
		self.entrada.grid(row=0, column=0, columnspan=4, padx=10, pady=10, sticky="we") # el sticky hace que ocupe todo el ancho
		self.scrollbar = tk.Scrollbar(self.frame, orient=tk.VERTICAL)
		self.historial = tk.Listbox(self.frame, width=16, height=5, font=("Courier", 16), yscrollcommand=self.scrollbar.set)
		self.historial.grid(row=6, column=0, columnspan=4, padx=10, pady=10, sticky="we")
		self.scrollbar.config(command=self.historial.yview)
		self.scrollbar.grid(row=6, column=4, sticky="ns", padx=(0,10), pady=10)
		self.historial.bind("<<ListboxSelect>>", lambda event: self.obtener_seleccion())
		self.display = display
		self.widgets = widgets
		self.httpClient = httpClient
		self.crear_botones()

	def crear_botones(self):
		# listas de listas con los botones
		botones = [
			("C", 1, 0), ("<-", 1, 1), ("", 1, 2), ("", 1, 3),
			('7', 2, 0), ('8', 2, 1), ('9', 2, 2), ('/', 2, 3),
			('4', 3, 0), ('5', 3, 1), ('6', 3, 2), ('*', 3, 3),
			('1', 4, 0), ('2', 4, 1), ('3', 4, 2), ('-', 4, 3),
			('0', 5, 0), ('.', 5, 1), ('=', 5, 2), ('+', 5, 3),
		]

		for (texto, fila, columna) in botones:
			if texto:  # solo crea botones si hay texto
				boton = tk.Button(self.frame, text=texto, width=5, height=2, font=("Courier", 18))
				boton.grid(row=fila, column=columna, padx=2, pady=2)
				boton.config(command=lambda t=texto: self.pulsar_boton(t))

	def pulsar_boton(self, texto):
		if texto == "C":
			self.entrada.delete(0, tk.END)
		elif texto == "<-":
			cadena = self.entrada.get()
			cadena = cadena[:-1]  # elimina el último carácter
			self.entrada.delete(0, tk.END)
			self.entrada.insert(0, cadena)
		elif texto == "=":
			try:
				resultado = eval(self.entrada.get())
				self.entrada.delete(0, tk.END)
				self.entrada.insert(tk.END, str(resultado))
				self.historial.insert(tk.END, resultado)
				# Mostrar sólo 2 dígitos en el display. Si la parte entera tiene
				# más de 2 dígitos mostramos "--" y enviamos "--" por HTTP.
				self.widgets.pausar_contador()  # Pausa cualquier conteo en curso
				try:
					# Usar la parte entera y su valor absoluto para contabilizar dígitos
					entero = abs(int(resultado))
				except Exception:
					# Si no es convertible a int (raro), fallback a mostrar error
					self.display.config(text="--")
					self.httpClient.request_mostrar_valor("--")
				else:
					entero_str = str(entero)
					if len(entero_str) > 2:
						# Más de 2 dígitos -> mostrar y enviar '--'
						self.display.config(text="--")
						self.httpClient.request_mostrar_valor("--")
					elif len(entero_str) == 1:
						# Un dígito -> mostrar con cero a la izquierda
						self.display.config(text="0" + entero_str)
						self.httpClient.request_mostrar_valor(int("0" + entero_str))
					else:
						# Dos dígitos -> mostrar tal cual y enviar valor entero
						self.display.config(text=entero_str[-2:])
						self.httpClient.request_mostrar_valor(int(entero_str[-2:]))
			except Exception as e:
				self.entrada.delete(0, tk.END)
				self.entrada.insert(tk.END, "Error")
		else:
			self.entrada.insert(tk.END, texto)

	def obtener_seleccion(self):
		# Esto es una tupla con los índices (= las posiciones)
		# de los ítems seleccionados por el usuario.
		indices = self.historial.curselection()
		if indices:
			valor = self.historial.get(indices[0])
			self.entrada.delete(0, tk.END)
			self.entrada.insert(0, valor)

	def insertar(self, x, y):
		self.frame.pack()
		self.frame.place(x=x, y=y)