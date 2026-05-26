import { createContext, useContext, useMemo, useState, useCallback, type ReactNode } from 'react'
import { useReadings } from './ReadingsContext'
import api from '../services/api'
import type { LatestReadings, SensorStatuses, SensorStatus } from '../types'

interface AlarmState {
  alarmActive: boolean
  alarmMessage: string | null
  statuses: SensorStatuses
  latest: LatestReadings
  deviceStatus: 'online' | 'offline'
  overrideActive: boolean
  currentDeviceId: number | null
  acknowledgeAlarm: () => Promise<void>
}

function deriveLatest(readings: ReturnType<typeof useReadings>['readings']): LatestReadings {
  const latest: LatestReadings = {
    temperature: null, humidity: null, gas: null, flame: null,
  }
  for (const r of readings) {
    const key = r.sensor_type as keyof LatestReadings
    if (!latest[key]) {
      latest[key] = { value: r.value, unit: r.unit, timestamp: r.timestamp }
    }
  }
  return latest
}

function deriveStatuses(latest: LatestReadings): SensorStatuses {
  const temp = latest.temperature?.value ?? 0
  const hum = latest.humidity?.value ?? 50
  const gas = latest.gas?.value ?? 0
  const flame = latest.flame?.value ?? 0

  return {
    temperature: temp > 45 ? 'critical' : temp >= 35 ? 'warning' : 'normal',
    humidity: (hum < 20 || hum > 80) ? 'warning' : 'normal',
    gas: gas > 300 ? 'critical' : gas >= 200 ? 'warning' : 'normal',
    flame: flame > 0 ? 'critical' : 'normal',
  }
}

function deriveDeviceStatus(readings: ReturnType<typeof useReadings>['readings']): 'online' | 'offline' {
  if (readings.length === 0) return 'offline'
  const latest = new Date(readings[0].timestamp).getTime()
  return (Date.now() - latest) < 35000 ? 'online' : 'offline'
}

const AlarmContext = createContext<AlarmState | null>(null)

export function AlarmProvider({ children }: { children: ReactNode }) {
  const { readings } = useReadings()
  const latest = useMemo(() => deriveLatest(readings), [readings])
  const statuses = useMemo(() => deriveStatuses(latest), [latest])
  const deviceStatus = useMemo(() => deriveDeviceStatus(readings), [readings])

  const [overrideActive, setOverrideActive] = useState(false)
  const currentDeviceId = readings.length > 0 ? readings[0].device : null

  const acknowledgeAlarm = useCallback(async () => {
    if (!currentDeviceId) return
    try {
      await api.post(`/devices/${currentDeviceId}/command/`, { command: 'acknowledge_alarm' })
    } catch {
      // Silently fail — api.ts interceptor handles 401
    }
  }, [currentDeviceId])

  const alarmActive = statuses.gas === 'critical' || statuses.flame === 'critical'
  const alarmMessage = alarmActive
    ? statuses.gas === 'critical'
      ? 'Gas crítico detectado (>300 PPM)'
      : 'Flama detectada'
    : null

  return (
    <AlarmContext.Provider value={{
      alarmActive, alarmMessage, statuses, latest, deviceStatus,
      overrideActive, currentDeviceId, acknowledgeAlarm,
    }}>
      {children}
    </AlarmContext.Provider>
  )
}

export function useAlarm() {
  const ctx = useContext(AlarmContext)
  if (!ctx) throw new Error('useAlarm must be used within AlarmProvider')
  return ctx
}
