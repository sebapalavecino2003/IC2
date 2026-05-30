/**
 * Página de inicio de sesión de NodeAlert IoT.
 *
 * Presenta un formulario de autenticación centrado con campos de
 * usuario y contraseña. Si el usuario ya está autenticado (sesión
 * previa), redirige automáticamente al dashboard para evitar que
 * vea la pantalla de login innecesariamente.
 *
 * Incluye manejo de errores con feedback visual mediante Snackbar,
 * mostrando mensajes descriptivos según la respuesta del servidor.
 */
import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import {
  Box, Card, CardContent, Typography, TextField, Button,
  CircularProgress, Snackbar, Alert,
} from '@mui/material'
import SensorsIcon from '@mui/icons-material/Sensors'
import { useAuth } from '../context/AuthContext'

export default function LoginPage() {
  const { user, login } = useAuth()
  const navigate = useNavigate()
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [loading, setLoading] = useState(false)
  const [snackbar, setSnackbar] = useState({ open: false, message: '', severity: 'error' })

  // Si ya hay sesión activa, redirigir al dashboard inmediatamente.
  if (user) {
    navigate('/', { replace: true })
    return null
  }

  const handleSubmit = async (e) => {
    e.preventDefault()
    if (!username || !password) {
      setSnackbar({ open: true, message: 'Completa todos los campos', severity: 'warning' })
      return
    }
    setLoading(true)
    try {
      await login(username, password)
      navigate('/', { replace: true })
    } catch (err) {
      // Extraer mensaje de error del backend con múltiples formatos
      // soportados por DRF (detail, non_field_errors).
      const msg = err.response?.data?.detail
        || err.response?.data?.non_field_errors?.[0]
        || 'Credenciales inválidas'
      setSnackbar({ open: true, message: msg, severity: 'error' })
    } finally {
      setLoading(false)
    }
  }

  return (
    <Box
      sx={{
        minHeight: '100vh',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        background: 'radial-gradient(ellipse at top, #0f1a2e 0%, #0a0f1a 70%)',
        p: 2,
      }}
    >
      <Card sx={{ maxWidth: 400, width: '100%', p: 2 }}>
        <CardContent sx={{ textAlign: 'center' }}>
          <SensorsIcon sx={{ fontSize: 48, color: '#00e5ff', mb: 1 }} />
          <Typography variant="h5" fontWeight={700} gutterBottom>
            NodeAlert IoT
          </Typography>
          <Typography variant="body2" color="text.secondary" sx={{ mb: 3 }}>
            Monitoreo Ambiental Inteligente
          </Typography>
          <Box component="form" onSubmit={handleSubmit}>
            <TextField
              fullWidth
              size="small"
              label="Usuario"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              sx={{ mb: 2 }}
              autoFocus
            />
            <TextField
              fullWidth
              size="small"
              label="Contraseña"
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              sx={{ mb: 3 }}
            />
            <Button
              fullWidth
              type="submit"
              variant="contained"
              size="large"
              disabled={loading}
              sx={{ py: 1.2 }}
            >
              {loading ? <CircularProgress size={24} /> : 'Iniciar Sesión'}
            </Button>
          </Box>
        </CardContent>
      </Card>
      <Snackbar
        open={snackbar.open}
        autoHideDuration={5000}
        onClose={() => setSnackbar((s) => ({ ...s, open: false }))}
        anchorOrigin={{ vertical: 'top', horizontal: 'center' }}
      >
        <Alert severity={snackbar.severity} variant="filled">
          {snackbar.message}
        </Alert>
      </Snackbar>
    </Box>
  )
}
