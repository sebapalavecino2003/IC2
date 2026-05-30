/**
 * Indicador visual tipo gauge para un sensor individual.
 *
 * Muestra el valor actual, la unidad y una barra de progreso coloreada
 * según la relación valor/máximo. El color cambia dinámicamente:
 * verde (normal), amarillo (warning), rojo (critical).
 *
 * Se utiliza en la página de detalle del dispositivo para dar una
 * vista rápida de cada métrica.
 */
import { Card, CardContent, Typography, Box, LinearProgress } from '@mui/material'
import { statusColor, sensorStatus } from '../utils/formatters'

export default function SensorGauge({ label, value, unit, max, icon: Icon }) {
  const status = sensorStatus(value, max)
  const color = statusColor(status)
  const pct = value != null ? Math.min((value / max) * 100, 100) : 0

  return (
    <Card sx={{ minWidth: 160, flex: 1 }}>
      <CardContent sx={{ textAlign: 'center', py: 2 }}>
        {Icon && <Icon sx={{ fontSize: 36, color, mb: 0.5 }} />}
        <Typography variant="body2" color="text.secondary" gutterBottom>
          {label}
        </Typography>
        <Typography variant="h4" sx={{ fontFamily: '"Roboto Mono", monospace', color, fontWeight: 700 }}>
          {value != null ? value.toFixed(1) : '—'}
        </Typography>
        <Typography variant="caption" color="text.secondary">
          {unit}
        </Typography>
        <Box sx={{ mt: 1 }}>
          <LinearProgress
            variant="determinate"
            value={pct}
            sx={{
              height: 6,
              borderRadius: 3,
              bgcolor: '#1e293b',
              '& .MuiLinearProgress-bar': {
                bgcolor: color,
                transition: 'transform 0.5s ease',
              },
            }}
          />
        </Box>
      </CardContent>
    </Card>
  )
}
