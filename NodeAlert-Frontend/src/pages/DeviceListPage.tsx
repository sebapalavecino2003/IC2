import { useEffect, useState } from 'react'
import { Container, Typography, Card, CardContent, Grid, Chip, Box, CircularProgress } from '@mui/material'
import { useNavigate } from 'react-router-dom'
import api from '../services/api'
import type { Device, PaginatedResponse } from '../types'

export default function DeviceListPage() {
  const navigate = useNavigate()
  const [devices, setDevices] = useState<Device[]>([])
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    api.get<PaginatedResponse<Device>>('/devices')
      .then((res) => setDevices(res.data.results))
      .catch(() => {})
      .finally(() => setLoading(false))
  }, [])

  if (loading) return <Box sx={{ display: 'flex', justifyContent: 'center', mt: 4 }}><CircularProgress /></Box>

  return (
    <Container maxWidth="lg" sx={{ py: 3 }}>
      <Typography variant="h4" gutterBottom>Dispositivos</Typography>
      {devices.length === 0 && <Typography color="text.secondary">No hay dispositivos registrados</Typography>}
      <Grid container spacing={2}>
        {devices.map((d) => (
          <Grid item xs={12} sm={6} md={4} key={d.id}>
            <Card sx={{ cursor: 'pointer' }} onClick={() => navigate(`/device/${d.id}`)}>
              <CardContent>
                <Typography variant="h6">{d.name}</Typography>
                <Typography variant="body2" color="text.secondary">ID: {d.device_id}</Typography>
                <Typography variant="body2" color="text.secondary">Ubicación: {d.location || '—'}</Typography>
                <Box sx={{ mt: 1 }}>
                  <Chip
                    label={d.is_active ? 'Activo' : 'Inactivo'}
                    color={d.is_active ? 'success' : 'default'}
                    size="small"
                  />
                </Box>
              </CardContent>
            </Card>
          </Grid>
        ))}
      </Grid>
    </Container>
  )
}
