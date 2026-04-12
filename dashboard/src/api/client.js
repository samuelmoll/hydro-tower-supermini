/**
 * API client for Hydroponic Tower backend
 */

import axios from 'axios'

// Create axios instance
const apiClient = axios.create({
  baseURL: '/api',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// API methods
export default {
  // Sensors
  getSensors(params = {}) {
    return apiClient.get('/sensors', { params })
  },

  getLatestSensor() {
    return apiClient.get('/sensors/latest')
  },

  getSensorStats(hours = 24) {
    return apiClient.get('/sensors/stats', { params: { hours } })
  },

  // Schedule
  getSchedules() {
    return apiClient.get('/schedule')
  },

  updateSchedules(schedules) {
    return apiClient.put('/schedule', { schedules })
  },

  addSchedule(schedule) {
    return apiClient.post('/schedule', schedule)
  },

  deleteSchedule(id) {
    return apiClient.delete(`/schedule/${id}`)
  },

  // Control
  controlPump(action, durationMin = null) {
    const data = { action }
    if (durationMin) data.duration_min = durationMin
    return apiClient.post('/pump/control', data)
  },

  getStatus() {
    return apiClient.get('/status')
  },

  getPumpHistory(limit = 50) {
    return apiClient.get('/pump/history', { params: { limit } })
  },

  updateSensorInterval(intervalSec) {
    return apiClient.post('/sensor/config', null, {
      params: { interval_sec: intervalSec }
    })
  },

  // Timezone
  getTimezone() {
    return apiClient.get('/timezone')
  },

  updateTimezone(timezone) {
    return apiClient.put('/timezone', { timezone })
  }
}
