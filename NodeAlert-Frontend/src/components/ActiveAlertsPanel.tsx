import { useState, useEffect } from 'react'
import {
  Box, Card, CardContent, Typography, Chip, IconButton, Alert as MuiAlert,
} from '@mui/material'
import VolumeOffIcon from '@mui/icons-material/VolumeOff'
import { useAlarm } from '../context/AlarmContext'
import { useReadings } from '../context/ReadingsContext'

const SEVERITY_COLORS: Record<string, 'info' | 'warning' | 'error'> = {
  info: 'info',
  warning: 'warning',
  critical: 'error',
}

const SEVERITY_LABELS: Record<string, string> = {
  info: 'Info',
  warning: 'Advertencia',
  critical: 'Crítico',
}

function elapsedTime(timestamp: string): string {
  const elapsed = Date.now() - new Date(timestamp).getTime()
  const mins = Math.floor(elapsed / 60000)
  const secs = Math.floor((elapsed % 60000) / 1000)
  if (mins > 0) return `${mins}m ${secs}s`
  return `${secs}s`
}

export default function ActiveAlertsPanel() {
  const { events } = useReadings()
  const { acknowledgeAlarm } = useAlarm()
  const [acknowledging, setAcknowledging] = useState(false)
  const [, setTick] = useState(0)

  useEffect(() => {
    const interval = setInterval(() => setTick(t => t + 1), 10000)
    return () => clearInterval(interval)
  }, [])

  const unresolved = events.filter(e => !e.resolved)

  const handleSilenceAlarm = async () => {
    setAcknowledging(true)
    await acknowledgeAlarm()
    setAcknowledging(false)
  }

  return (
    <Card>
      <CardContent>
        <Typography variant="h6" gutterBottom>
          Alertas Activas ({unresolved.length})
        </Typography>
        {unresolved.length === 0 ? (
          <MuiAlert severity="success">No hay alertas activas</MuiAlert>
        ) : (
          unresolved.map((event) => (
            <Box
              key={event.id}
              sx={{
                display: 'flex', justifyContent: 'space-between', alignItems: 'center',
                mb: 1, p: 1.5, borderRadius: 1,
                bgcolor: event.severity === 'critical' ? 'error.dark' :
                         event.severity === 'warning' ? 'warning.dark' : 'transparent',
              }}
            >
              <Box>
                <Chip
                  label={SEVERITY_LABELS[event.severity] || event.severity}
                  color={SEVERITY_COLORS[event.severity]}
                  size="small"
                  sx={{ mr: 1 }}
                />
                <Typography variant="body2" component="span">
                  {event.message}
                </Typography>
                <Typography variant="caption" display="block" sx={{ mt: 0.5, opacity: 0.7 }}>
                  {elapsedTime(event.timestamp)} atrás
                </Typography>
              </Box>
              <IconButton
                size="small"
                onClick={handleSilenceAlarm}
                disabled={acknowledging}
                title="Silenciar alarma"
              >
                <VolumeOffIcon fontSize="small" />
              </IconButton>
            </Box>
          ))
        )}
      </CardContent>
    </Card>
  )
}
