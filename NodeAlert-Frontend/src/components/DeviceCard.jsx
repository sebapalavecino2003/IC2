/**
 * Tarjeta resumen de un dispositivo en el dashboard.
 *
 * Muestra el estado online/offline, nombre, ID y las últimas lecturas
 * de sensores (temperatura, humedad, gas, flama). El borde izquierdo
 * cambia de color según el estado. Al hacer clic, navega a la página
 * de detalle del dispositivo.
 *
 * Si el dispositivo tiene alertas activas, muestra un badge contador.
 */
import { Card, CardContent, Typography, Box, Chip } from '@mui/material'
import MemoryIcon from '@mui/icons-material/Memory'
import { useNavigate } from 'react-router-dom'
import { deviceStatus, statusColor, formatValue } from '../utils/formatters'

export default function DeviceCard({ device, readings, events }) {
  const navigate = useNavigate()
  const status = deviceStatus(device.last_seen)
  const color = statusColor(status)

  const deviceReadings = readings?.find((r) => r.device === device.id)
  const activeAlerts = events?.filter((e) => !e.resolved && e.device === device.id)?.length || 0

  return (
    <Card
      sx={{
        cursor: 'pointer',
        transition: 'all 0.2s',
        borderLeft: `4px solid ${color}`,
        '&:hover': {
          transform: 'translateY(-2px)',
          boxShadow: `0 4px 20px ${color}30`,
        },
      }}
      onClick={() => navigate(`/device/${device.device_id}`)}
    >
      <CardContent>
        <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', mb: 1 }}>
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
            <MemoryIcon sx={{ color }} />
            <Box>
              <Typography variant="subtitle1" fontWeight={600}>
                {device.name || device.device_id}
              </Typography>
              <Typography variant="caption" color="text.secondary">
                {device.device_id}
              </Typography>
            </Box>
          </Box>
          <Chip
            label={status}
            size="small"
            sx={{
              bgcolor: `${color}20`,
              color,
              fontWeight: 600,
              border: `1px solid ${color}`,
            }}
          />
        </Box>
        <Box sx={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 0.5, mt: 1 }}>
          <Typography variant="caption" color="text.secondary">Temperatura</Typography>
          <Typography variant="body2" sx={{ fontFamily: '"Roboto Mono", monospace' }} align="right">
            {formatValue(deviceReadings?.temperature, '°C')}
          </Typography>
          <Typography variant="caption" color="text.secondary">Humedad</Typography>
          <Typography variant="body2" sx={{ fontFamily: '"Roboto Mono", monospace' }} align="right">
            {formatValue(deviceReadings?.humidity, '%')}
          </Typography>
          <Typography variant="caption" color="text.secondary">Gas</Typography>
          <Typography variant="body2" sx={{ fontFamily: '"Roboto Mono", monospace' }} align="right">
            {formatValue(deviceReadings?.gas_ppm, 'ppm')}
          </Typography>
          <Typography variant="caption" color="text.secondary">Flama</Typography>
          <Typography variant="body2" sx={{ fontFamily: '"Roboto Mono", monospace' }} align="right">
            {formatValue(deviceReadings?.flame, 'ADC')}
          </Typography>
        </Box>
        {activeAlerts > 0 && (
          <Chip
            label={`${activeAlerts} alerta${activeAlerts > 1 ? 's' : ''}`}
            size="small"
            color="error"
            sx={{ mt: 1 }}
          />
        )}
      </CardContent>
    </Card>
  )
}
