/**
 * Panel de configuración remota de umbrales del dispositivo.
 *
 * Permite al operador ajustar los límites de alarma (temperatura
 * máxima, gas máximo, flama máxima) y el intervalo de muestreo
 * del dispositivo. Los cambios se envían al ESP32 mediante comandos
 * MQTT (update_thresholds).
 *
 * Los valores se representan con sliders para ajuste visual rápido
 * y un campo numérico para el intervalo de muestreo.
 */
import { useState } from 'react'
import {
  Card, CardContent, Typography, Slider, Button,
  Box, Snackbar, Alert,
} from '@mui/material'
import { sendCommand } from '../services/api'

export default function DeviceConfig({ deviceId }) {
  const [tempMax, setTempMax] = useState(50)
  const [gasMax, setGasMax] = useState(2000)
  const [flameMax, setFlameMax] = useState(2000)
  const [loading, setLoading] = useState(false)
  const [snackbar, setSnackbar] = useState({ open: false, message: '', severity: 'success' })

  const handleSaveThresholds = async () => {
    setLoading(true)
    try {
      await sendCommand(deviceId, 'update_thresholds', {
        temp_max: tempMax,
        gas_max: gasMax,
        flame_max: flameMax,
      })
      setSnackbar({ open: true, message: 'Umbrales actualizados', severity: 'success' })
    } catch {
      setSnackbar({ open: true, message: 'Error al actualizar umbrales', severity: 'error' })
    } finally {
      setLoading(false)
    }
  }

  return (
    <>
      <Card>
        <CardContent>
          <Typography variant="h6" gutterBottom>Umbrales</Typography>
          <Box sx={{ mb: 2 }}>
            <Typography variant="body2" gutterBottom>
              Temperatura máxima: {tempMax}°C
            </Typography>
            <Slider value={tempMax} onChange={(_, v) => setTempMax(v)} min={30} max={80} step={1} />
          </Box>
          <Box sx={{ mb: 2 }}>
            <Typography variant="body2" gutterBottom>
              Gas máximo: {gasMax} ADC
            </Typography>
            <Slider value={gasMax} onChange={(_, v) => setGasMax(v)} min={500} max={4000} step={100} />
          </Box>
          <Box sx={{ mb: 2 }}>
            <Typography variant="body2" gutterBottom>
              Flama máxima: {flameMax} ADC
            </Typography>
            <Slider value={flameMax} onChange={(_, v) => setFlameMax(v)} min={500} max={4000} step={100} />
          </Box>
          <Button variant="contained" fullWidth onClick={handleSaveThresholds} disabled={loading}>
            Actualizar Umbrales
          </Button>
        </CardContent>
      </Card>
      <Snackbar
        open={snackbar.open}
        autoHideDuration={4000}
        onClose={() => setSnackbar((s) => ({ ...s, open: false }))}
      >
        <Alert severity={snackbar.severity} variant="filled">
          {snackbar.message}
        </Alert>
      </Snackbar>
    </>
  )
}
