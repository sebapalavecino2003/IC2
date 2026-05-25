export interface Device {
  id: number
  device_id: string
  name: string
  mac_address: string
  location: string
  is_active: boolean
  created_at: string
  updated_at: string
}

export interface Reading {
  id: number
  device: number
  sensor_type: 'temperature' | 'humidity' | 'gas' | 'flame'
  value: number
  unit: string
  timestamp: string
}

export interface Event {
  id: number
  device: number
  event_type: string
  severity: 'info' | 'warning' | 'critical'
  message: string
  resolved: boolean
  timestamp: string
}

export interface LatestReadings {
  temperature: { value: number; unit: string; timestamp: string } | null
  humidity: { value: number; unit: string; timestamp: string } | null
  gas: { value: number; unit: string; timestamp: string } | null
  flame: { value: number; unit: string; timestamp: string } | null
}

export type SensorStatus = 'normal' | 'warning' | 'critical'
export type DeviceStatus = 'online' | 'offline'

export interface SensorStatuses {
  temperature: SensorStatus
  humidity: SensorStatus
  gas: SensorStatus
  flame: SensorStatus
}
