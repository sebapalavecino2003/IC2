import { useState, useMemo } from 'react'
import {
  Table, TableBody, TableCell, TableContainer, TableHead, TableRow,
  Chip, IconButton, Select, MenuItem, FormControl, InputLabel, Box,
} from '@mui/material'
import CheckCircleIcon from '@mui/icons-material/CheckCircle'
import type { Event } from '../types'

interface EventTableProps {
  events: Event[]
  onResolve: (id: number) => void
}

const SEVERITY_COLORS: Record<string, 'info' | 'warning' | 'error'> = {
  info: 'info',
  warning: 'warning',
  critical: 'error',
}

const EVENT_TYPE_LABELS: Record<string, string> = {
  'threshold_crossed': 'Umbral superado',
  'actuator_on': 'Actuador ON',
  'actuator_off': 'Actuador OFF',
  'override': 'Override',
  'thresholds': 'Configuración',
  'flame_detected': 'Llama detectada',
  'gas_alert': 'Alerta de gas',
}

export default function EventTable({ events, onResolve }: EventTableProps) {
  const [filterSeverity, setFilterSeverity] = useState<string>('all')

  const filtered = useMemo(() => {
    if (filterSeverity === 'all') return events
    return events.filter((e) => e.severity === filterSeverity)
  }, [events, filterSeverity])

  return (
    <Box>
      <FormControl size="small" sx={{ mb: 2, minWidth: 120 }}>
        <InputLabel>Severidad</InputLabel>
        <Select value={filterSeverity} label="Severidad" onChange={(e) => setFilterSeverity(e.target.value)}>
          <MenuItem value="all">Todas</MenuItem>
          <MenuItem value="info">Info</MenuItem>
          <MenuItem value="warning">Warning</MenuItem>
          <MenuItem value="critical">Critical</MenuItem>
        </Select>
      </FormControl>
      <TableContainer>
        <Table size="small">
          <TableHead>
            <TableRow>
              <TableCell>Timestamp</TableCell>
              <TableCell>Severidad</TableCell>
              <TableCell>Tipo</TableCell>
              <TableCell>Mensaje</TableCell>
              <TableCell>Acción</TableCell>
            </TableRow>
          </TableHead>
          <TableBody>
            {filtered.length === 0 && (
              <TableRow>
                <TableCell colSpan={5} align="center">No hay eventos registrados</TableCell>
              </TableRow>
            )}
            {filtered.map((event) => (
              <TableRow key={event.id}>
                <TableCell>{new Date(event.timestamp).toLocaleString()}</TableCell>
                <TableCell>
                  <Chip label={event.severity} color={SEVERITY_COLORS[event.severity]} size="small" />
                </TableCell>
                <TableCell>{EVENT_TYPE_LABELS[event.event_type] || event.event_type}</TableCell>
                <TableCell>{event.message}</TableCell>
                <TableCell>
                  {!event.resolved && (
                    <IconButton size="small" onClick={() => onResolve(event.id)} title="Marcar resuelto">
                      <CheckCircleIcon fontSize="small" />
                    </IconButton>
                  )}
                </TableCell>
              </TableRow>
            ))}
          </TableBody>
        </Table>
      </TableContainer>
    </Box>
  )
}
