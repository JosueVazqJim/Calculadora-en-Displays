import requests

class HTTPClient:
	def __init__(self, ip):
		self.ip = ip
		self.base_url = f"http://{self.ip}"

	def request_conteo(self, direccion, valor):
		url = f"{self.base_url}/conteo/{direccion}/{valor}"
		print(url)
		try:
			response = requests.get(url, timeout=2)
			response.raise_for_status()
			return response.text
		except Exception as e:
			print(f"Error HTTP GET: {e}")
			return None
		
	def request_pausar_conteo(self):
		url = f"{self.base_url}/conteo/stop"
		print(url)
		try:
			response = requests.get(url, timeout=2)
			response.raise_for_status()
			return response.text
		except Exception as e:
			print(f"Error HTTP GET: {e}")
			return None

	def request_reiniciar_conteo(self):
		url = f"{self.base_url}/conteo/restart"
		print(url)
		try:
			response = requests.get(url, timeout=2)
			response.raise_for_status()
			return response.text
		except Exception as e:
			print(f"Error HTTP GET: {e}")
			return None
		
	def request_mostrar_valor(self, valor):
		url = f"{self.base_url}/display/{valor}"
		print(url)
		try:
			response = requests.get(url, timeout=2)
			response.raise_for_status()
			return response.text
		except Exception as e:
			print(f"Error HTTP GET: {e}")
			return None
		
	def pussyDestruction(self):
		url = f"{self.base_url}/destruction"
		try:
			response = requests.get(url, timeout=2)
			response.raise_for_status()
			return response.text
		except Exception as e:
			print(f"Error HTTP GET: {e}")
			return None