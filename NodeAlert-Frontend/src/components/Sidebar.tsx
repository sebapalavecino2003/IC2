import { Drawer, List, ListItem, ListItemButton, ListItemIcon, ListItemText, Toolbar } from '@mui/material'
import DashboardIcon from '@mui/icons-material/Dashboard'
import DevicesIcon from '@mui/icons-material/Devices'
import { useNavigate } from 'react-router-dom'

const DRAWER_WIDTH = 220

export default function Sidebar() {
  const navigate = useNavigate()

  return (
    <Drawer
      variant="permanent"
      sx={{
        width: DRAWER_WIDTH,
        flexShrink: 0,
        '& .MuiDrawer-paper': { width: DRAWER_WIDTH, boxSizing: 'border-box' },
      }}
    >
      <Toolbar />
      <List>
        <ListItem disablePadding>
          <ListItemButton onClick={() => navigate('/')}>
            <ListItemIcon><DashboardIcon /></ListItemIcon>
            <ListItemText primary="Dashboard" />
          </ListItemButton>
        </ListItem>
        <ListItem disablePadding>
          <ListItemButton onClick={() => navigate('/devices')}>
            <ListItemIcon><DevicesIcon /></ListItemIcon>
            <ListItemText primary="Dispositivos" />
          </ListItemButton>
        </ListItem>
      </List>
    </Drawer>
  )
}

/* Re-export Toolbar from MUI for layout spacing */
export { DRAWER_WIDTH }
