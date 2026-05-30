/**
 * Tabla paginada del historial de eventos de un dispositivo.
 *
 * Muestra todos los eventos (resueltos y activos) con columnas de
 * fecha, tipo, severidad, mensaje, estado y acción. Soporta:
 *   - Paginación en servidor (page_size, page).
 *   - Ordenamiento por fecha ascendente/descendente.
 *   - Acción para resolver eventos directamente desde la tabla.
 */
import { useState, useEffect } from 'react'
import {
  Card, CardContent, Typography, Table, TableBody, TableCell,
  TableContainer, TableHead, TableRow, IconButton, Chip,
  TablePagination, Box, TableSortLabel,
} from '@mui/material'
import CheckCircleIcon from '@mui/icons-material/CheckCircle'
import { getEvents, updateEvent } from '../services/api'
import { formatTimestamp } from '../utils/formatters'

const severityColor = { critical: 'error', warning: 'warning', info: 'info' }

const eventTypeLabels = {
  gas_leak: 'Fuga de gas',
  flame_detected: 'Llama detectada',
  threshold_exceeded: 'Umbral superado',
  buzzer_on: 'Buzzer ON',
  buzzer_off: 'Buzzer OFF',
  override: 'Override',
  config_change: 'Configuración',
  acknowledge_alarm: 'Alarma',
}

export default function EventHistory({ deviceId }) {
  const [events, setEvents] = useState([])
  const [page, setPage] = useState(0)
  const [rowsPerPage, setRowsPerPage] = useState(10)
  const [total, setTotal] = useState(0)
  const [order, setOrder] = useState('desc')

  const fetchEvents = async () => {
    const params = {
      page: page + 1,
      page_size: rowsPerPage,
      ordering: `${order === 'desc' ? '-' : ''}timestamp`,
    }
    if (deviceId) params.device_id = deviceId
    const { data } = await getEvents(params)
    setEvents(Array.isArray(data) ? data : data.results || [])
    setTotal(data.count || (Array.isArray(data) ? data.length : 0))
  }

  useEffect(() => { fetchEvents() }, [page, rowsPerPage, order, deviceId])

  const handleResolve = async (id) => {
    await updateEvent(id, { resolved: true })
    fetchEvents()
  }

  return (
    <Card>
      <CardContent>
        <Typography variant="h6" gutterBottom>Historial de Eventos</Typography>
        <TableContainer>
          <Table size="small">
            <TableHead>
              <TableRow>
                <TableCell>
                  <TableSortLabel
                    active
                    direction={order}
                    onClick={() => setOrder(order === 'desc' ? 'asc' : 'desc')}
                  >
                    Fecha
                  </TableSortLabel>
                </TableCell>
                <TableCell>Tipo</TableCell>
                <TableCell>Severidad</TableCell>
                <TableCell>Mensaje</TableCell>
                <TableCell>Estado</TableCell>
                <TableCell>Acción</TableCell>
              </TableRow>
            </TableHead>
            <TableBody>
              {events.map((ev) => (
                <TableRow key={ev.id}>
                  <TableCell sx={{ fontFamily: '"Roboto Mono", monospace', fontSize: 12 }}>
                    {formatTimestamp(ev.timestamp)}
                  </TableCell>
                  <TableCell>{eventTypeLabels[ev.event_type] || ev.event_type}</TableCell>
                  <TableCell>
                    <Chip
                      label={ev.severity}
                      size="small"
                      color={severityColor[ev.severity] || 'default'}
                      sx={{ height: 20, fontSize: 10 }}
                    />
                  </TableCell>
                  <TableCell>{ev.message || '—'}</TableCell>
                  <TableCell>{ev.resolved ? 'Resuelto' : 'Activo'}</TableCell>
                  <TableCell>
                    {!ev.resolved && (
                      <IconButton size="small" onClick={() => handleResolve(ev.id)}>
                        <CheckCircleIcon fontSize="small" />
                      </IconButton>
                    )}
                  </TableCell>
                </TableRow>
              ))}
              {!events.length && (
                <TableRow>
                  <TableCell colSpan={6} align="center">
                    <Typography variant="body2" color="text.secondary">
                      No hay eventos registrados
                    </Typography>
                  </TableCell>
                </TableRow>
              )}
            </TableBody>
          </Table>
        </TableContainer>
        <TablePagination
          component="div"
          count={total}
          page={page}
          onPageChange={(_, p) => setPage(p)}
          rowsPerPage={rowsPerPage}
          onRowsPerPageChange={(e) => { setRowsPerPage(parseInt(e.target.value, 10)); setPage(0) }}
          labelRowsPerPage="Por página:"
        />
      </CardContent>
    </Card>
  )
}
