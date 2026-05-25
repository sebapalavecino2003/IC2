import { Box } from '@mui/material'
import AppBar from '../components/AppBar'
import Sidebar, { DRAWER_WIDTH } from '../components/Sidebar'
import SummaryBar from '../components/SummaryBar'
import ChartsPanel from '../components/ChartsPanel'
import AlertPanel from '../components/AlertPanel'
import AlarmSound from '../components/AlarmSound'

export default function DashboardPage() {
  return (
    <Box sx={{ display: 'flex' }}>
      <AlarmSound />
      <AppBar />
      <Sidebar />
      <Box
        component="main"
        sx={{
          flexGrow: 1,
          ml: `${DRAWER_WIDTH}px`,
          mt: '64px',
          p: 3,
        }}
      >
        <SummaryBar />
        <ChartsPanel />
        <AlertPanel />
      </Box>
    </Box>
  )
}
