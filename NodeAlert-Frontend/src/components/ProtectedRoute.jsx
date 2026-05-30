/**
 * Componente de protección de rutas.
 *
 * Envuelve componentes que requieren autenticación. Mientras se
 * verifica la sesión (loading), muestra un spinner de carga.
 * Si el usuario no está autenticado, redirige a /login.
 * Si está autenticado, renderiza los children.
 */
import { Navigate } from 'react-router-dom'
import { CircularProgress, Box } from '@mui/material'
import { useAuth } from '../context/AuthContext'

export default function ProtectedRoute({ children }) {
  const { user, loading } = useAuth()

  if (loading) {
    return (
      <Box sx={{ display: 'flex', justifyContent: 'center', alignItems: 'center', height: '100vh' }}>
        <CircularProgress />
      </Box>
    )
  }

  if (!user) return <Navigate to="/login" replace />
  return children
}
