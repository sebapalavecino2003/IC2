import MuiAppBar from '@mui/material/AppBar'
import Toolbar from '@mui/material/Toolbar'
import Typography from '@mui/material/Typography'
import Chip from '@mui/material/Chip'
import Button from '@mui/material/Button'
import Box from '@mui/material/Box'
import { useAuth } from '../context/AuthContext'
import { useAlarm } from '../context/AlarmContext'

export default function AppBar() {
  const { userInfo, logout, isAdmin } = useAuth()
  const { alarmActive } = useAlarm()

  return (
    <MuiAppBar position="static" sx={alarmActive ? { backgroundColor: '#3A1B1B' } : undefined}>
      <Toolbar>
        <Typography variant="h6" sx={{ flexGrow: 1, fontFamily: "'Roboto Mono', monospace" }}>
          NodeAlert IoT
        </Typography>
        {userInfo && (
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mr: 2 }}>
            <Typography variant="body2">{userInfo.username}</Typography>
            <Chip
              label={isAdmin ? 'Admin' : 'Analyst'}
              size="small"
              color={isAdmin ? 'primary' : 'default'}
            />
          </Box>
        )}
        <Button color="inherit" onClick={logout}>Cerrar Sesión</Button>
      </Toolbar>
    </MuiAppBar>
  )
}
