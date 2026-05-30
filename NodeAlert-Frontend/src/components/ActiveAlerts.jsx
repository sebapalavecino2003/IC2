/**
 * Panel de alertas activas del sistema.
 *
 * Muestra una lista de eventos no resueltos con su severidad, tipo
 * y tiempo transcurrido. Permite al operador:
 *   - Silenciar la alarma del dispositivo (acknowledge_alarm).
 *   - Marcar eventos como resueltos (resolved).
 *   - Filtrar por severidad (todas, critical, warning, info).
 *
 * Los datos se refrescan automáticamente cada 5 segundos mediante
 * un intervalo interno, independientemente del polling del dashboard.
 */
import { useState, useEffect } from 'react'
import {
  Card, CardContent, Typography, List, ListItem, ListItemText,
  ListItemIcon, IconButton, Chip, Box, Alert,
} from '@mui/material'
import CheckCircleIcon from '@mui/icons-material/CheckCircle'
import WarningAmberIcon from '@mui/icons-material/WarningAmber'
import InfoIcon from '@mui/icons-material/Info'
import ErrorIcon from '@mui/icons-material/Error'
import VolumeOffIcon from '@mui/icons-material/VolumeOff'
import { getEvents, updateEvent, sendCommand } from '../services/api'
import { timeAgo, formatTimestamp } from '../utils/formatters'

const severityIcon = {
  critical: <ErrorIcon sx={{ color: '#ff1744' }} />,
  warning: <WarningAmberIcon sx={{ color: '#ffd600' }} />,
  info: <InfoIcon sx={{ color: '#2979ff' }} />,
}

const severityColor = {
  critical: 'error',
  warning: 'warning',
  info: 'info',
}

const eventTypeLabels = {
  gas_leak: 'Fuga de gas',
  flame_detected: 'Llama detectada',
  threshold_exceeded: 'Umbral superado',
  buzzer_on: 'Buzzer ON',
  buzzer_off: 'Buzzer OFF',
  override: 'Override',
  config_change: 'Configuración',
  acknowledge_alarm: 'Alarma silenciada',
}

export default function ActiveAlerts({ deviceId }) {
  const [alerts, setAlerts] = useState([])
  const [filter, setFilter] = useState('all')

  const fetchAlerts = async () => {
    try {
      const params = { resolved: false, ordering: '-timestamp' }
      if (deviceId) params.device_id = deviceId
      const { data } = await getEvents(params)
      setAlerts(Array.isArray(data) ? data : data.results || [])
    } catch {
      // Fallo silencioso: el intervalo reintentará.
    }
  }

  useEffect(() => {
    fetchAlerts()
    const id = setInterval(fetchAlerts, 5000)
    return () => clearInterval(id)
  }, [deviceId])

  const handleResolve = async (id) => {
    await updateEvent(id, { resolved: true })
    fetchAlerts()
  }

  const handleSilence = async (id, deviceId) => {
    await sendCommand(deviceId, 'acknowledge_alarm')
    fetchAlerts()
  }

  const filtered = filter === 'all'
    ? alerts
    : alerts.filter((a) => a.severity === filter)

  if (!alerts.length) {
    return (
      <Card>
        <CardContent>
          <Typography variant="h6" gutterBottom>Alertas Activas</Typography>
          <Alert severity="success" sx={{ bgcolor: '#00e67615' }}>
            No hay alertas activas
          </Alert>
        </CardContent>
      </Card>
    )
  }

  return (
    <Card>
      <CardContent>
        <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 1 }}>
          <Typography variant="h6">Alertas Activas</Typography>
          <Box sx={{ display: 'flex', gap: 0.5 }}>
            {['all', 'critical', 'warning', 'info'].map((s) => (
              <Chip
                key={s}
                label={s === 'all' ? 'Todas' : s}
                size="small"
                variant={filter === s ? 'filled' : 'outlined'}
                color={severityColor[s] || 'default'}
                onClick={() => setFilter(s)}
              />
            ))}
          </Box>
        </Box>
        <List dense>
          {filtered.map((alert) => (
            <ListItem
              key={alert.id}
              secondaryAction={
                <Box sx={{ display: 'flex', gap: 0.5 }}>
                  <IconButton
                    size="small"
                    onClick={() => handleSilence(alert.id, alert.device_id)}
                    title="Silenciar alarma"
                  >
                    <VolumeOffIcon fontSize="small" />
                  </IconButton>
                  <IconButton
                    size="small"
                    onClick={() => handleResolve(alert.id)}
                    title="Marcar resuelto"
                  >
                    <CheckCircleIcon fontSize="small" />
                  </IconButton>
                </Box>
              }
            >
              <ListItemIcon sx={{ minWidth: 40 }}>
                {severityIcon[alert.severity] || <InfoIcon />}
              </ListItemIcon>
              <ListItemText
                primary={
                  <Box sx={{ display: 'flex', gap: 1, alignItems: 'center' }}>
                    <Typography variant="body2">
                      {eventTypeLabels[alert.event_type] || alert.event_type}
                    </Typography>
                    <Chip
                      label={alert.severity}
                      size="small"
                      color={severityColor[alert.severity] || 'default'}
                      sx={{ height: 20, fontSize: 10 }}
                    />
                    <Typography variant="caption" color="text.secondary">
                      {timeAgo(alert.timestamp)}
                    </Typography>
                  </Box>
                }
                secondary={alert.message || formatTimestamp(alert.timestamp)}
              />
            </ListItem>
          ))}
        </List>
      </CardContent>
    </Card>
  )
}
