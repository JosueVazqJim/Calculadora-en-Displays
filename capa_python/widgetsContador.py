import random
import tkinter as tk


#solo widgets de Contador, sera incrustado en la interfaz principal
class WidgetsContador:
	def __init__(self, master, display, httpClient):
		self.master = master  # es el tk principal
		self.frame = tk.Frame(master, bg="gray")
		self.frame.grid_columnconfigure(0, weight=1)
		self.display = display
		self.httpClient = httpClient
		self.titulo = tk.Label(self.frame, text="Controles del Display", font=("Courier", 20, "bold"), bg="gray", fg="white")
		self.titulo.grid(row=0, column=0, columnspan=4, pady=20)
		self.crear_botones()
		self.contando = ""

	def crear_botones(self):
		# listas de listas con los botones
		botones = [
			("Iniciar conteo ASCENDENTE", 1, 0),
			('Iniciar conteo DESCENDENTE', 2, 0),
			('Pausar contador', 3, 0), 
			('Reiniciar contador', 4, 0), 
		]

		for (texto, fila, columna) in botones:
			if texto:  # solo crea botones si hay texto
				boton = tk.Button(self.frame, text=texto, width=27, height=2, font=("Courier", 18))
				boton.grid(row=fila, column=columna, pady=2, sticky="ew")
				# boton.config(command=lambda t=texto: self.pulsar_boton(t))

				if texto == "Iniciar conteo ASCENDENTE":
					boton.config(command=self.conteo_ascendente)
				elif texto == "Iniciar conteo DESCENDENTE":
					boton.config(command=self.conteo_descendente)
				elif texto == "Pausar contador":
					boton.config(command=self.pausar_contador)
				elif texto == "Reiniciar contador":
					boton.config(command=self.reiniciar_contador)

	def conteo_ascendente(self):
		if self.contando:
			self.pausar_contador()  # Pausa cualquier conteo en curso
		self.httpClient.request_conteo("up", self._display_to_int())
		self._conteo_ascendente()

	def _conteo_ascendente(self):
		numero_actual = self._display_to_int()
		if numero_actual >= 99:
			self.display.config(text="00")  # Muestra "00"
			self.pausar_contador()          # Detiene el conteo
			return

		siguiente_numero = numero_actual + 1
		self.display.config(text=f"{siguiente_numero:02d}")
		self.contando = self.master.after(1000, self._conteo_ascendente)

	def conteo_descendente(self):
		if self.contando:
			self.pausar_contador()  # Pausa cualquier conteo en curso
		self.httpClient.request_conteo("down", self._display_to_int())
		self._conteo_descendente()

	def _conteo_descendente(self):
		numero_actual = self._display_to_int()
		if numero_actual <= 0:
			self.display.config(text="00")
			self.pausar_contador()
			return

		anterior_numero = numero_actual - 1
		self.display.config(text=f"{anterior_numero:02d}")
		self.contando = self.master.after(1000, self._conteo_descendente)

	def pausar_contador(self):
		
		if self.contando:
			self.master.after_cancel(self.contando)
			self.httpClient.request_pausar_conteo()

	def reiniciar_contador(self):
		if self.contando:
			self.pausar_contador()
		self.display.config(text="00")
		self.httpClient.request_reiniciar_conteo()

	def _display_to_int(self):
		"""Devuelve el valor numérico mostrado en el display.
		Si el display contiene '--' o no es numérico, retorna 0."""
		text = str(self.display.cget("text")).strip()
		if text == "--":
			return 0
		try:
			return int(text)
		except Exception:
			# Fallback por seguridad
			return 0

	def insertar(self, x, y):
		self.frame.pack()
		self.frame.place(x=x, y=y)