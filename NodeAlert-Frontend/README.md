# NodeAlert IoT — Frontend React

## Stack

- **Framework:** React 18 con TypeScript
- **Build tool:** Vite
- **UI:** Material UI (MUI)
- **Despliegue:** Build multi-stage Docker con nginx

## Variables de Entorno

| Variable | Descripción |
|----------|-------------|
| `VITE_API_URL` | URL base de la API REST (ej: `http://localhost:80/api/v1`) |

## Desarrollo

```bash
# Instalar dependencias
npm install

# Iniciar servidor de desarrollo
npm run dev

# Abrir en navegador
# http://localhost:5173
```

## Producción

El frontend se despliega como parte del stack Docker Compose:

```bash
docker compose up -d
```

El build multi-stage Docker genera archivos estáticos servidos por nginx en el puerto 3000 del host.
