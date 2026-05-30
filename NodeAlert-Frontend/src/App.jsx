/**
 * Componente raíz de la aplicación NodeAlert IoT.
 *
 * Define la estructura de enrutamiento completa:
 *   - /login: Página de inicio de sesión pública.
 *   - /: Dashboard protegido con layout principal (sidebar + appbar).
 *   - /device/:id: Página de detalle de dispositivo también protegida.
 *   - *: Cualquier ruta no definida redirige al dashboard.
 *
 * El AuthProvider envuelve todas las rutas para que el contexto de
 * autenticación esté disponible globalmente.
 */
import { Routes, Route, Navigate } from 'react-router-dom'
import { AuthProvider } from './context/AuthContext'
import ProtectedRoute from './components/ProtectedRoute'
import MainLayout from './layouts/MainLayout'
import LoginPage from './pages/LoginPage'
import DashboardPage from './pages/DashboardPage'
import DeviceDetailPage from './pages/DeviceDetailPage'

export default function App() {
  return (
    <AuthProvider>
      <Routes>
        <Route path="/login" element={<LoginPage />} />
        <Route
          path="/"
          element={
            <ProtectedRoute>
              <MainLayout />
            </ProtectedRoute>
          }
        >
          <Route index element={<DashboardPage />} />
          <Route path="device/:id" element={<DeviceDetailPage />} />
        </Route>
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </AuthProvider>
  )
}
