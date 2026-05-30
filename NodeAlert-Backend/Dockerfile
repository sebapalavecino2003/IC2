# Dockerfile para el backend Django NodeAlert.
# Utiliza python:3.12-slim para imagen reducida e instala dependencias
# del sistema necesarias para mysqlclient.
FROM python:3.12-slim

# Instala dependencias del sistema para compilar mysqlclient.
# gcc, default-libmysqlclient-dev y pkg-config son necesarios para
# compilar el wheel de mysqlclient durante pip install.
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    default-libmysqlclient-dev \
    pkg-config \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Instala dependencias Python desde requirements.txt.
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copia el código fuente y el entrypoint.
COPY . .
COPY entrypoint.sh .
RUN chmod +x entrypoint.sh

EXPOSE 8000

CMD ["./entrypoint.sh"]
