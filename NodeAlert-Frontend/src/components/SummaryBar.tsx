import { Grid, Chip, Box } from '@mui/material'
import BuildIcon from '@mui/icons-material/Build'
import PlayArrowIcon from '@mui/icons-material/PlayArrow'
import SensorGauge from './SensorGauge'
import { useAlarm } from '../context/AlarmContext'

export default function SummaryBar() {
  const { latest, statuses, deviceStatus, alarmActive, alarmMessage, overrideActive } = useAlarm()

  return (
    <Box sx={{ mb: 2 }}>
      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 1 }}>
        <Box sx={{ display: 'flex', gap: 1, alignItems: 'center' }}>
          <Chip
            label={deviceStatus === 'online' ? 'ESP32 Conectado' : 'ESP32 Desconectado'}
            color={deviceStatus === 'online' ? 'success' : 'error'}
            size="small"
            variant="outlined"
          />
          {deviceStatus === 'online' && (
            <Chip
              label={overrideActive ? 'Override Manual' : 'Modo Auto'}
              color={overrideActive ? 'warning' : 'success'}
              size="small"
              variant={overrideActive ? 'filled' : 'outlined'}
              icon={overrideActive ? <BuildIcon /> : <PlayArrowIcon />}
            />
          )}
        </Box>
        {alarmActive && alarmMessage && (
          <Chip
            label={alarmMessage}
            color="error"
            size="small"
            sx={{ animation: 'blink 1s infinite' }}
          />
        )}
      </Box>
      <Grid container spacing={2}>
        <Grid item xs={6} sm={3}>
          <SensorGauge label="Temperatura" value={latest.temperature?.value ?? null} unit={latest.temperature?.unit ?? '°C'} status={statuses.temperature} />
        </Grid>
        <Grid item xs={6} sm={3}>
          <SensorGauge label="Humedad" value={latest.humidity?.value ?? null} unit={latest.humidity?.unit ?? '%'} status={statuses.humidity} />
        </Grid>
        <Grid item xs={6} sm={3}>
          <SensorGauge label="Gas" value={latest.gas?.value ?? null} unit={latest.gas?.unit ?? 'ADC'} status={statuses.gas} />
        </Grid>
        <Grid item xs={6} sm={3}>
          <SensorGauge label="Llama" value={latest.flame?.value ?? null} unit={latest.flame?.unit ?? 'ADC'} status={statuses.flame} />
        </Grid>
      </Grid>
    </Box>
  )
}
