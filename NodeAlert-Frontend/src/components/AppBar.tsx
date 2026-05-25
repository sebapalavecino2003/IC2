import MuiAppBar from '@mui/material/AppBar'
import Toolbar from '@mui/material/Toolbar'
import Typography from '@mui/material/Typography'
import Button from '@mui/material/Button'
import { useAuth } from '../context/AuthContext'
import { useAlarm } from '../context/AlarmContext'

export default function AppBar() {
  const { logout } = useAuth()
  const { alarmActive } = useAlarm()

  return (
    <MuiAppBar position="static" sx={alarmActive ? { backgroundColor: '#3A1B1B' } : undefined}>
      <Toolbar>
        <Typography variant="h6" sx={{ flexGrow: 1, fontFamily: "'Roboto Mono', monospace" }}>
          NodeAlert IoT
        </Typography>
        <Button color="inherit" onClick={logout}>Cerrar Sesión</Button>
      </Toolbar>
    </MuiAppBar>
  )
}
