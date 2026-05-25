import { Container, Typography } from '@mui/material'
import SummaryBar from '../components/SummaryBar'

export default function DashboardPage() {
  return (
    <Container maxWidth="xl" sx={{ py: 3 }}>
      <Typography variant="h4" gutterBottom sx={{ fontWeight: 500 }}>
        Panel de Monitoreo
      </Typography>
      <SummaryBar />
    </Container>
  )
}
