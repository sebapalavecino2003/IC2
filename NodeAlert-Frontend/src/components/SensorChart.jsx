/**
 * Gráfico de área para visualizar series temporales de sensores.
 *
 * Utiliza Recharts (AreaChart) para renderizar datos de lecturas con
 * gradiente de color, tooltips interactivos y ejes con formato de
 * hora local. Incluye selectores de rango temporal (1H, 6H, 24H, 7D)
 * para filtrar la ventana de datos visible.
 */
import { useState } from 'react'
import {
  Card, CardContent, Typography, Box, ToggleButtonGroup, ToggleButton,
} from '@mui/material'
import {
  AreaChart, Area, XAxis, YAxis, Tooltip, ResponsiveContainer, CartesianGrid,
} from 'recharts'
const RANGES = [
  { label: '1H', value: 1 },
  { label: '6H', value: 6 },
  { label: '24H', value: 24 },
  { label: '7D', value: 168 },
]

export default function SensorChart({ title, dataKey, color, data, unit }) {
  const [range, setRange] = useState(1)

  const filtered = (data || []).filter((d) => {
    if (!d.timestamp) return true
    const hours = (Date.now() - new Date(d.timestamp).getTime()) / 3600000
    return hours <= range
  })

  return (
    <Card>
      <CardContent>
        <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
          <Typography variant="h6">{title}</Typography>
          <ToggleButtonGroup
            size="small"
            value={range}
            exclusive
            onChange={(_, v) => v && setRange(v)}
          >
            {RANGES.map((r) => (
              <ToggleButton key={r.value} value={r.value} sx={{ px: 1.5 }}>
                {r.label}
              </ToggleButton>
            ))}
          </ToggleButtonGroup>
        </Box>
        <ResponsiveContainer width="100%" height={250}>
          <AreaChart data={filtered}>
            <defs>
              <linearGradient id={`grad-${dataKey}`} x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor={color} stopOpacity={0.3} />
                <stop offset="95%" stopColor={color} stopOpacity={0} />
              </linearGradient>
            </defs>
            <CartesianGrid strokeDasharray="3 3" stroke="#1e293b" />
            <XAxis
              dataKey="timestamp"
              tick={{ fontSize: 11, fill: '#94a3b8' }}
              tickFormatter={(ts) => {
                if (!ts) return ''
                return new Date(ts).toLocaleTimeString('es-ES', { hour: '2-digit', minute: '2-digit' })
              }}
              stroke="#334155"
            />
            <YAxis tick={{ fontSize: 11, fill: '#94a3b8' }} stroke="#334155" unit={unit ? ` ${unit}` : ''} />
            <Tooltip
              contentStyle={{
                background: '#111827',
                border: '1px solid #1e293b',
                borderRadius: 8,
                color: '#e0e6ed',
              }}
              labelFormatter={(ts) => ts ? new Date(ts).toLocaleString('es-ES') : ''}
              formatter={(val) => [`${Number(val).toFixed(1)} ${unit || ''}`, title]}
            />
            <Area
              type="monotone"
              dataKey={dataKey}
              stroke={color}
              strokeWidth={2}
              fill={`url(#grad-${dataKey})`}
              dot={false}
            />
          </AreaChart>
        </ResponsiveContainer>
      </CardContent>
    </Card>
  )
}
