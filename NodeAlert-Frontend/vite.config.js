/**
 * Configuración de Vite para el frontend NodeAlert.
 *
 * Define el plugin de React para transformación JSX y un proxy de
 * desarrollo que redirige las solicitudes /api al backend Django
 * en localhost:8000, evitando problemas de CORS durante el desarrollo.
 */
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/api': {
        target: 'http://localhost:8000',
        changeOrigin: true,
      },
    },
  },
})
