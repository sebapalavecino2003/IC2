import { Card, CardContent, Typography } from '@mui/material'
import type { SensorStatus } from '../types'

interface SensorGaugeProps {
  label: string
  value: number | null
  unit: string
  status: SensorStatus
}

const STATUS_COLORS: Record<SensorStatus, string> = {
  normal: '#4CAF50',
  warning: '#FFC107',
  critical: '#F44336',
}

const STATUS_BG: Record<SensorStatus, string> = {
  normal: '#1B3A1B',
  warning: '#3A2E1B',
  critical: '#3A1B1B',
}

export default function SensorGauge({ label, value, unit, status }: SensorGaugeProps) {
  const color = STATUS_COLORS[status]
  const bg = STATUS_BG[status]
  const isCritical = status === 'critical'

  return (
    <Card
      sx={{
        bg,
        border: `1px solid ${color}`,
        animation: isCritical ? 'blink 1s infinite' : undefined,
        '@keyframes blink': {
          '0%, 100%': { opacity: 1 },
          '50%': { opacity: 0.3 },
        },
      }}
    >
      <CardContent sx={{ textAlign: 'center', py: 2 }}>
        <Typography variant="overline" color={color} fontWeight="bold">
          {label}
        </Typography>
        <Typography variant="h4" color={color} sx={{ my: 1 }}>
          {value !== null ? value : '—'}
        </Typography>
        <Typography variant="body2" color={color}>
          {value !== null ? unit : 'Sin datos'}
        </Typography>
      </CardContent>
    </Card>
  )
}
