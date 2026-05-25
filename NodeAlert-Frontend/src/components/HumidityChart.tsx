import { Card, CardContent, Typography } from '@mui/material'
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts'
import type { Reading } from '../types'

interface HumidityChartProps {
  data: Reading[]
}

export default function HumidityChart({ data }: HumidityChartProps) {
  const chartData = data
    .filter((r) => r.sensor_type === 'humidity')
    .slice(0, 100)
    .reverse()
    .map((r) => ({
      time: new Date(r.timestamp).toLocaleTimeString(),
      humedad: r.value,
    }))

  return (
    <Card>
      <CardContent>
        <Typography variant="subtitle1" gutterBottom>Humedad (%)</Typography>
        <ResponsiveContainer width="100%" height={250}>
          <LineChart data={chartData}>
            <CartesianGrid strokeDasharray="3 3" stroke="#30363D" />
            <XAxis dataKey="time" stroke="#8B949E" fontSize={11} />
            <YAxis domain={[0, 100]} stroke="#8B949E" fontSize={11} />
            <Tooltip
              contentStyle={{ backgroundColor: '#161B22', border: '1px solid #30363D' }}
              labelStyle={{ color: '#E6EDF3' }}
            />
            <Line type="monotone" dataKey="humedad" stroke="#03A9F4" strokeWidth={2} dot={false} />
          </LineChart>
        </ResponsiveContainer>
      </CardContent>
    </Card>
  )
}
