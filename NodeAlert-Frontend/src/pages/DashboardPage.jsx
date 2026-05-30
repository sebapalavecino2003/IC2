/**
 * Página principal del dashboard de monitoreo.
 *
 * Muestra una vista general del sistema con:
 *   - Widgets de estadísticas (nodos online, alertas activas, promedios).
 *   - Cuadrícula de tarjetas de dispositivos con búsqueda y filtros.
 *   - Gráficos de sensores (temperatura, humedad, gas).
 *   - Panel de alertas activas.
 *
 * Los datos se actualizan automáticamente cada 3 segundos mediante
 * el hook usePolling, permitiendo monitoreo en tiempo real sin
 * necesidad de recargar la página.
 */
import { useState, useCallback } from 'react'
import {
  Box, Grid, Typography, TextField, InputAdornment, Chip, Stack,
} from '@mui/material'
import SensorsIcon from '@mui/icons-material/Sensors'
import WarningAmberIcon from '@mui/icons-material/WarningAmber'
import ErrorIcon from '@mui/icons-material/Error'
import DeviceThermostatIcon from '@mui/icons-material/DeviceThermostat'
import WaterDropIcon from '@mui/icons-material/WaterDrop'
import SearchIcon from '@mui/icons-material/Search'
import usePolling from '../hooks/usePolling'
import { getDevices, getReadings, getEvents } from '../services/api'
import { deviceStatus, statusColor, groupLatestReadings, readingsForChart } from '../utils/formatters'
import DeviceCard from '../components/DeviceCard'
import ActiveAlerts from '../components/ActiveAlerts'
import SensorChart from '../components/SensorChart'

/**
 * Widget de estadística rectangular.
 * Muestra un icono, etiqueta y valor numérico con color semántico.
 */
function StatWidget({ icon, label, value, color }) {
  return (
    <Box
      sx={{
        bgcolor: '#111827',
        border: '1px solid #1e293b',
        borderRadius: 2,
        p: 2,
        display: 'flex',
        alignItems: 'center',
        gap: 2,
        minWidth: 160,
        flex: 1,
      }}
    >
      <Box sx={{ color, display: 'flex' }}>{icon}</Box>
      <Box>
        <Typography variant="caption" color="text.secondary">{label}</Typography>
        <Typography variant="h5" sx={{ fontWeight: 700, color }}>{value}</Typography>
      </Box>
    </Box>
  )
}

export default function DashboardPage() {
  const [devices, setDevices] = useState([])
  const [readings, setReadings] = useState([])
  const [events, setEvents] = useState([])
  const [search, setSearch] = useState('')
  const [filter, setFilter] = useState('all')

  // Polling cada 3 segundos para datos en tiempo real.
  usePolling(
    useCallback(async () => {
      try {
        const [devRes, readRes, evRes] = await Promise.all([
          getDevices(),
          getReadings({ ordering: '-timestamp' }),
          getEvents({ resolved: false, ordering: '-timestamp' }),
        ])
        setDevices(Array.isArray(devRes.data) ? devRes.data : devRes.data?.results || [])
        setReadings(Array.isArray(readRes.data) ? readRes.data : readRes.data?.results || [])
        setEvents(Array.isArray(evRes.data) ? evRes.data : evRes.data?.results || [])
      } catch {
        // Fallo silencioso: el polling reintentará en el próximo ciclo.
      }
    }, []),
    3000,
  )

  // Calcular estado (online/offline) para cada dispositivo.
  const devicesWithStatus = devices.map((d) => ({
    ...d,
    _status: deviceStatus(d.last_seen),
  }))

  const onlineCount = devicesWithStatus.filter((d) => d._status === 'online').length
  const activeAlertsCount = events.filter((e) => !e.resolved).length
  const criticalCount = events.filter((e) => e.severity === 'critical' && !e.resolved).length

  // Promedios de temperatura y humedad desde las últimas lecturas.
  const tempReadings = readings.filter((r) => r.sensor_type === 'temperature')
  const humReadings = readings.filter((r) => r.sensor_type === 'humidity')
  const avgTemp = tempReadings.length
    ? (tempReadings.reduce((s, r) => s + r.value, 0) / tempReadings.length).toFixed(1)
    : '—'
  const avgHum = humReadings.length
    ? (humReadings.reduce((s, r) => s + r.value, 0) / humReadings.length).toFixed(1)
    : '—'

  // Filtrar dispositivos por búsqueda textual y estado.
  const filteredDevices = devicesWithStatus.filter((d) => {
    const matchesSearch = search
      ? (d.name || '').toLowerCase().includes(search.toLowerCase())
        || (d.device_id || '').toLowerCase().includes(search.toLowerCase())
        || (d.location || '').toLowerCase().includes(search.toLowerCase())
      : true
    const matchesFilter = filter === 'all' || d._status === filter
    return matchesSearch && matchesFilter
  })

  // Últimas lecturas por dispositivo en formato plano para las tarjetas.
  const latestReadings = groupLatestReadings(readings)

  return (
    <Box>
      <Typography variant="h5" sx={{ mb: 3, fontWeight: 700 }}>
        Dashboard
      </Typography>

      <Stack direction="row" flexWrap="wrap" spacing={2} sx={{ mb: 3 }}>
        <StatWidget
          icon={<SensorsIcon sx={{ fontSize: 32 }} />}
          label="Nodos Online"
          value={`${onlineCount}/${devices.length}`}
          color="#00e676"
        />
        <StatWidget
          icon={<WarningAmberIcon sx={{ fontSize: 32 }} />}
          label="Alertas Activas"
          value={activeAlertsCount}
          color="#ffd600"
        />
        <StatWidget
          icon={<ErrorIcon sx={{ fontSize: 32 }} />}
          label="Eventos Críticos"
          value={criticalCount}
          color="#ff1744"
        />
        <StatWidget
          icon={<DeviceThermostatIcon sx={{ fontSize: 32 }} />}
          label="Temp. Promedio"
          value={avgTemp !== '—' ? `${avgTemp}°C` : avgTemp}
          color="#2979ff"
        />
        <StatWidget
          icon={<WaterDropIcon sx={{ fontSize: 32 }} />}
          label="Hum. Promedio"
          value={avgHum !== '—' ? `${avgHum}%` : avgHum}
          color="#00e5ff"
        />
      </Stack>

      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2, flexWrap: 'wrap', gap: 1 }}>
        <TextField
          size="small"
          placeholder="Buscar dispositivo..."
          value={search}
          onChange={(e) => setSearch(e.target.value)}
          sx={{ minWidth: 260 }}
          InputProps={{
            startAdornment: (
              <InputAdornment position="start">
                <SearchIcon fontSize="small" />
              </InputAdornment>
            ),
          }}
        />
        <Stack direction="row" spacing={0.5}>
          {['all', 'online', 'offline', 'critical', 'warning'].map((s) => (
            <Chip
              key={s}
              label={s === 'all' ? 'Todos' : s}
              size="small"
              variant={filter === s ? 'filled' : 'outlined'}
              onClick={() => setFilter(s)}
              sx={{
                color: filter === s ? undefined : statusColor(s),
                borderColor: statusColor(s),
                bgcolor: filter === s ? `${statusColor(s)}20` : undefined,
              }}
            />
          ))}
        </Stack>
      </Box>

      <Grid container spacing={2} sx={{ mb: 3 }}>
        {filteredDevices.map((device) => (
          <Grid item xs={12} sm={6} md={4} lg={3} key={device.id || device.device_id}>
            <DeviceCard device={device} readings={latestReadings} events={events} />
          </Grid>
        ))}
        {!filteredDevices.length && (
          <Grid item xs={12}>
            <Typography variant="body1" color="text.secondary" sx={{ textAlign: 'center', py: 4 }}>
              No se encontraron dispositivos
            </Typography>
          </Grid>
        )}
      </Grid>

      <Grid container spacing={2}>
        <Grid item xs={12} md={8}>
          <SensorChart
            title="Temperatura"
            dataKey="temperature"
            color="#ff6f00"
            data={readingsForChart(readings, 'temperature').slice(0, 100)}
            unit="°C"
          />
        </Grid>
        <Grid item xs={12} md={4}>
          <ActiveAlerts />
        </Grid>
        <Grid item xs={12} md={6}>
          <SensorChart
            title="Humedad"
            dataKey="humidity"
            color="#00e5ff"
            data={readingsForChart(readings, 'humidity').slice(0, 100)}
            unit="%"
          />
        </Grid>
        <Grid item xs={12} md={6}>
          <SensorChart
            title="Gas (ppm)"
            dataKey="gas_ppm"
            color="#ff1744"
            data={readingsForChart(readings, 'gas_ppm').slice(0, 100)}
            unit="ppm"
          />
        </Grid>
      </Grid>
    </Box>
  )
}
