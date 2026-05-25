import { useState, useMemo } from 'react'
import { Grid, Card, CardContent, Typography } from '@mui/material'
import TimeFilter, { type TimeRange } from './TimeFilter'
import TempChart from './TempChart'
import HumidityChart from './HumidityChart'
import { useReadings } from '../context/ReadingsContext'

function getTimeFilterMs(range: TimeRange): number {
  const now = Date.now()
  switch (range) {
    case '1h': return now - 3600000
    case '6h': return now - 6 * 3600000
    case '24h': return now - 24 * 3600000
    case '7d': return now - 7 * 86400000
  }
}

export default function ChartsPanel() {
  const { readings } = useReadings()
  const [timeRange, setTimeRange] = useState<TimeRange>('1h')

  const filteredReadings = useMemo(() => {
    const cutoff = getTimeFilterMs(timeRange)
    return readings.filter((r) => new Date(r.timestamp).getTime() >= cutoff)
  }, [readings, timeRange])

  return (
    <Card sx={{ mb: 2 }}>
      <CardContent>
        <Typography variant="h6" gutterBottom>Historial de Lecturas</Typography>
        <TimeFilter value={timeRange} onChange={setTimeRange} />
        <Grid container spacing={2}>
          <Grid item xs={12} md={6}>
            <TempChart data={filteredReadings} />
          </Grid>
          <Grid item xs={12} md={6}>
            <HumidityChart data={filteredReadings} />
          </Grid>
        </Grid>
      </CardContent>
    </Card>
  )
}
