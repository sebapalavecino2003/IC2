/**
 * Panel de acciones remotas para un dispositivo.
 *
 * Proporciona botones para enviar comandos directos al ESP32:
 * Reboot, Buzzer ON, Buzzer OFF y modo Automático. Cada acción
 * requiere confirmación mediante un diálogo modal para prevenir
 * envíos accidentales.
 *
 * Los comandos se envían a través del endpoint /devices/{id}/command/
 * del backend, que los publica en el topic MQTT correspondiente.
 */
import { useState } from 'react'
import {
  Card, CardContent, Typography, Button, ButtonGroup, Dialog,
  DialogTitle, DialogContent, DialogContentText, DialogActions, Snackbar, Alert,
} from '@mui/material'
import RestartAltIcon from '@mui/icons-material/RestartAlt'
import VolumeUpIcon from '@mui/icons-material/VolumeUp'
import VolumeOffIcon from '@mui/icons-material/VolumeOff'
import PlayArrowIcon from '@mui/icons-material/PlayArrow'
import { sendCommand } from '../services/api'

const COMMANDS = [
  { label: 'Reboot', icon: <RestartAltIcon />, command: 'reboot', color: 'warning' },
  { label: 'Buzzer ON', icon: <VolumeUpIcon />, command: 'buzzer_on', color: 'error' },
  { label: 'Buzzer OFF', icon: <VolumeOffIcon />, command: 'buzzer_off', color: 'default' },
  { label: 'Auto', icon: <PlayArrowIcon />, command: 'return_to_auto', color: 'success' },
]

export default function DeviceActions({ deviceId }) {
  const [confirmOpen, setConfirmOpen] = useState(false)
  const [pendingCmd, setPendingCmd] = useState(null)
  const [loading, setLoading] = useState(false)
  const [snackbar, setSnackbar] = useState({ open: false, message: '', severity: 'success' })

  const handleClick = (cmd) => {
    setPendingCmd(cmd)
    setConfirmOpen(true)
  }

  const handleConfirm = async () => {
    setLoading(true)
    setConfirmOpen(false)
    try {
      await sendCommand(deviceId, pendingCmd.command)
      setSnackbar({ open: true, message: `Comando "${pendingCmd.label}" ejecutado`, severity: 'success' })
    } catch {
      setSnackbar({ open: true, message: 'Error al ejecutar comando', severity: 'error' })
    } finally {
      setLoading(false)
      setPendingCmd(null)
    }
  }

  return (
    <>
      <Card>
        <CardContent>
          <Typography variant="h6" gutterBottom>Acciones Remotas</Typography>
          <ButtonGroup variant="outlined" fullWidth orientation="vertical">
            {COMMANDS.map((cmd) => (
              <Button
                key={cmd.command}
                startIcon={cmd.icon}
                color={cmd.color}
                onClick={() => handleClick(cmd)}
                disabled={loading}
              >
                {cmd.label}
              </Button>
            ))}
          </ButtonGroup>
        </CardContent>
      </Card>
      <Dialog open={confirmOpen} onClose={() => setConfirmOpen(false)}>
        <DialogTitle>Confirmar comando</DialogTitle>
        <DialogContent>
          <DialogContentText>
            ¿Enviar comando <strong>{pendingCmd?.label}</strong> al dispositivo?
          </DialogContentText>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setConfirmOpen(false)}>Cancelar</Button>
          <Button onClick={handleConfirm} variant="contained" color="primary">
            Confirmar
          </Button>
        </DialogActions>
      </Dialog>
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
