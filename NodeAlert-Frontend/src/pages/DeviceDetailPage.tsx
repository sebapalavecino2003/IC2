import { useEffect, useState } from 'react'
import { Container, Typography, Card, CardContent, Grid, Box, CircularProgress } from '@mui/material'
import { useParams } from 'react-router-dom'
import TempChart from '../components/TempChart'
import HumidityChart from '../components/HumidityChart'
import api from '../services/api'
import type { Device, Reading, PaginatedResponse } from '../types'

export default function DeviceDetailPage() {
  const { id } = useParams()
  const [device, setDevice] = useState<Device | null>(null)
  const [readings, setReadings] = useState<Reading[]>([])
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    Promise.all([
      api.get<Device>(`/devices/${id}`),
      api.get<PaginatedResponse<Reading>>('/readings', { params: { device_id: id, ordering: '-timestamp' } }),
    ])
      .then(([d, r]) => {
        setDevice(d.data)
        setReadings(r.data.results)
      })
      .catch(() => {})
      .finally(() => setLoading(false))
  }, [id])

  if (loading) return <Box sx={{ display: 'flex', justifyContent: 'center', mt: 4 }}><CircularProgress /></Box>
  if (!device) return <Container><Typography>Dispositivo no encontrado</Typography></Container>

  return (
    <Container maxWidth="lg" sx={{ py: 3 }}>
      <Typography variant="h4" gutterBottom>{device.name}</Typography>
      <Typography variant="body2" color="text.secondary" gutterBottom>
        ID: {device.device_id} | MAC: {device.mac_address || '—'} | Ubicación: {device.location || '—'}
      </Typography>
      <Grid container spacing={2} sx={{ mt: 1 }}>
        <Grid item xs={12} md={6}>
          <TempChart data={readings} />
        </Grid>
        <Grid item xs={12} md={6}>
          <HumidityChart data={readings} />
        </Grid>
      </Grid>
    </Container>
  )
}
