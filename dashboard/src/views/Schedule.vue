<template>
  <div class="space-y-6">
    <!-- Page header -->
    <div class="flex justify-between items-center">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-white">Pump Schedule</h1>
      <button @click="addSchedule" class="btn btn-primary flex items-center">
        <svg class="w-4 h-4 mr-2" fill="none" stroke="currentColor" viewBox="0 0 24 24">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 4v16m8-8H4" />
        </svg>
        Add Schedule
      </button>
    </div>

    <!-- Schedule list -->
    <div class="card">
      <div v-if="loading" class="text-center py-8 text-gray-500">
        Loading schedules...
      </div>
      
      <div v-else-if="schedules.length === 0" class="text-center py-8">
        <p class="text-gray-500 dark:text-gray-400 mb-4">No schedules configured</p>
        <button @click="addSchedule" class="btn btn-primary">
          Create First Schedule
        </button>
      </div>

      <div v-else class="space-y-4">
        <div
          v-for="(schedule, index) in schedules"
          :key="schedule.id || index"
          class="flex items-center justify-between p-4 bg-gray-50 dark:bg-gray-700 rounded-lg"
        >
          <div class="flex items-center space-x-4">
            <!-- Enable/disable toggle -->
            <button
              @click="toggleSchedule(index)"
              class="w-12 h-6 rounded-full transition-colors"
              :class="schedule.enabled ? 'bg-hydro-500' : 'bg-gray-300 dark:bg-gray-600'"
            >
              <span
                class="block w-5 h-5 bg-white rounded-full shadow transform transition-transform"
                :class="schedule.enabled ? 'translate-x-6' : 'translate-x-0.5'"
              ></span>
            </button>

            <!-- Time display -->
            <div>
              <p class="text-xl font-semibold text-gray-900 dark:text-white">
                {{ formatTime(schedule.hour, schedule.minute) }}
              </p>
              <p class="text-sm text-gray-500 dark:text-gray-400">
                Duration: {{ schedule.duration_min }} minutes
              </p>
            </div>
          </div>

          <!-- Actions -->
          <div class="flex items-center space-x-2">
            <button
              @click="editSchedule(index)"
              class="p-2 text-gray-500 hover:text-gray-700 dark:hover:text-gray-300"
            >
              <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" 
                      d="M11 5H6a2 2 0 00-2 2v11a2 2 0 002 2h11a2 2 0 002-2v-5m-1.414-9.414a2 2 0 112.828 2.828L11.828 15H9v-2.828l8.586-8.586z" />
              </svg>
            </button>
            <button
              @click="removeSchedule(index)"
              class="p-2 text-red-500 hover:text-red-700"
            >
              <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" 
                      d="M19 7l-.867 12.142A2 2 0 0116.138 21H7.862a2 2 0 01-1.995-1.858L5 7m5 4v6m4-6v6m1-10V4a1 1 0 00-1-1h-4a1 1 0 00-1 1v3M4 7h16" />
              </svg>
            </button>
          </div>
        </div>
      </div>

      <!-- Save button -->
      <div v-if="hasChanges" class="mt-6 pt-4 border-t border-gray-200 dark:border-gray-600">
        <button
          @click="saveSchedules"
          :disabled="saving"
          class="btn btn-primary w-full"
        >
          {{ saving ? 'Saving...' : 'Save Changes' }}
        </button>
      </div>
    </div>

    <!-- Timezone settings -->
    <div class="card">
      <h2 class="text-lg font-semibold text-gray-900 dark:text-white mb-4">Timezone Settings</h2>
      
      <div class="space-y-4">
        <div>
          <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
            Timezone
          </label>
          <select v-model="selectedTimezone" class="input w-full md:w-auto">
            <optgroup label="Americas">
              <option value="America/New_York">Eastern Time (New York)</option>
              <option value="America/Chicago">Central Time (Chicago)</option>
              <option value="America/Denver">Mountain Time (Denver)</option>
              <option value="America/Los_Angeles">Pacific Time (Los Angeles)</option>
              <option value="America/Anchorage">Alaska Time</option>
              <option value="Pacific/Honolulu">Hawaii Time</option>
              <option value="America/Toronto">Toronto</option>
              <option value="America/Vancouver">Vancouver</option>
              <option value="America/Sao_Paulo">São Paulo</option>
              <option value="America/Mexico_City">Mexico City</option>
            </optgroup>
            <optgroup label="Europe">
              <option value="Europe/London">London (GMT/BST)</option>
              <option value="Europe/Paris">Paris (CET)</option>
              <option value="Europe/Berlin">Berlin (CET)</option>
              <option value="Europe/Amsterdam">Amsterdam (CET)</option>
              <option value="Europe/Madrid">Madrid (CET)</option>
              <option value="Europe/Rome">Rome (CET)</option>
              <option value="Europe/Moscow">Moscow</option>
            </optgroup>
            <optgroup label="Asia Pacific">
              <option value="Asia/Dubai">Dubai</option>
              <option value="Asia/Kolkata">India (Kolkata)</option>
              <option value="Asia/Singapore">Singapore</option>
              <option value="Asia/Hong_Kong">Hong Kong</option>
              <option value="Asia/Shanghai">Shanghai</option>
              <option value="Asia/Tokyo">Tokyo</option>
              <option value="Asia/Seoul">Seoul</option>
            </optgroup>
            <optgroup label="Australia / Oceania">
              <option value="Australia/Perth">Perth (AWST)</option>
              <option value="Australia/Adelaide">Adelaide (ACST)</option>
              <option value="Australia/Sydney">Sydney (AEST)</option>
              <option value="Australia/Brisbane">Brisbane (AEST - No DST)</option>
              <option value="Australia/Melbourne">Melbourne (AEST)</option>
              <option value="Pacific/Auckland">Auckland (NZST)</option>
            </optgroup>
            <optgroup label="Other">
              <option value="UTC">UTC</option>
            </optgroup>
          </select>
        </div>

        <div v-if="currentTimezone" class="text-sm text-gray-500 dark:text-gray-400">
          <p>Current: {{ currentTimezone.timezone }}</p>
          <p>UTC Offset: {{ formatUtcOffset(currentTimezone.gmt_offset_sec) }}</p>
        </div>

        <div v-if="selectedTimezone !== currentTimezone?.timezone" class="pt-2">
          <button
            @click="saveTimezone"
            :disabled="savingTimezone"
            class="btn btn-primary"
          >
            {{ savingTimezone ? 'Saving...' : 'Update Timezone' }}
          </button>
        </div>

        <p v-if="timezoneMessage" class="text-sm" :class="timezoneSuccess ? 'text-green-600' : 'text-red-600'">
          {{ timezoneMessage }}
        </p>
      </div>
    </div>

    <!-- Edit modal -->
    <div
      v-if="showModal"
      class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50"
      @click.self="closeModal"
    >
      <div class="bg-white dark:bg-gray-800 rounded-xl p-6 w-full max-w-md mx-4">
        <h3 class="text-lg font-semibold text-gray-900 dark:text-white mb-4">
          {{ editingIndex === -1 ? 'Add Schedule' : 'Edit Schedule' }}
        </h3>

        <div class="space-y-4">
          <!-- Time picker -->
          <div class="grid grid-cols-2 gap-4">
            <div>
              <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
                Hour
              </label>
              <select v-model.number="editingSchedule.hour" class="input">
                <option v-for="h in 24" :key="h-1" :value="h-1">
                  {{ String(h-1).padStart(2, '0') }}
                </option>
              </select>
            </div>
            <div>
              <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
                Minute
              </label>
              <select v-model.number="editingSchedule.minute" class="input">
                <option v-for="m in [0, 15, 30, 45]" :key="m" :value="m">
                  {{ String(m).padStart(2, '0') }}
                </option>
              </select>
            </div>
          </div>

          <!-- Duration -->
          <div>
            <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
              Duration (minutes)
            </label>
            <input
              type="number"
              v-model.number="editingSchedule.duration_min"
              min="1"
              max="120"
              class="input"
            />
          </div>

          <!-- Enabled -->
          <div class="flex items-center">
            <input
              type="checkbox"
              v-model="editingSchedule.enabled"
              class="w-4 h-4 text-hydro-500 rounded"
              id="schedule-enabled"
            />
            <label for="schedule-enabled" class="ml-2 text-sm text-gray-700 dark:text-gray-300">
              Enabled
            </label>
          </div>
        </div>

        <div class="flex justify-end space-x-3 mt-6">
          <button @click="closeModal" class="btn btn-secondary">Cancel</button>
          <button @click="confirmEdit" class="btn btn-primary">
            {{ editingIndex === -1 ? 'Add' : 'Update' }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { ref, computed, onMounted } from 'vue'
import api from '../api/client'

export default {
  name: 'Schedule',
  setup() {
    const schedules = ref([])
    const originalSchedules = ref([])
    const loading = ref(true)
    const saving = ref(false)
    const showModal = ref(false)
    const editingIndex = ref(-1)
    const editingSchedule = ref({
      hour: 8,
      minute: 0,
      duration_min: 5,
      enabled: true
    })

    // Timezone state
    const currentTimezone = ref(null)
    const selectedTimezone = ref('UTC')
    const savingTimezone = ref(false)
    const timezoneMessage = ref('')
    const timezoneSuccess = ref(false)

    const hasChanges = computed(() => {
      return JSON.stringify(schedules.value) !== JSON.stringify(originalSchedules.value)
    })

    const fetchSchedules = async () => {
      loading.value = true
      try {
        const response = await api.getSchedules()
        schedules.value = response.data
        originalSchedules.value = JSON.parse(JSON.stringify(response.data))
      } catch (error) {
        console.error('Failed to fetch schedules:', error)
      } finally {
        loading.value = false
      }
    }

    const formatTime = (hour, minute) => {
      return `${String(hour).padStart(2, '0')}:${String(minute).padStart(2, '0')}`
    }

    const formatUtcOffset = (seconds) => {
      if (seconds === null || seconds === undefined) return 'Unknown'
      const hours = Math.floor(Math.abs(seconds) / 3600)
      const mins = Math.floor((Math.abs(seconds) % 3600) / 60)
      const sign = seconds >= 0 ? '+' : '-'
      return mins > 0 ? `UTC${sign}${hours}:${String(mins).padStart(2, '0')}` : `UTC${sign}${hours}`
    }

    const fetchTimezone = async () => {
      try {
        const response = await api.getTimezone()
        currentTimezone.value = response.data
        selectedTimezone.value = response.data.timezone
      } catch (error) {
        console.error('Failed to fetch timezone:', error)
      }
    }

    const saveTimezone = async () => {
      savingTimezone.value = true
      timezoneMessage.value = ''
      try {
        const response = await api.updateTimezone(selectedTimezone.value)
        currentTimezone.value = response.data
        timezoneMessage.value = 'Timezone updated successfully'
        timezoneSuccess.value = true
      } catch (error) {
        timezoneMessage.value = error.response?.data?.detail || 'Failed to update timezone'
        timezoneSuccess.value = false
      } finally {
        savingTimezone.value = false
        // Clear message after 3 seconds
        setTimeout(() => { timezoneMessage.value = '' }, 3000)
      }
    }

    const addSchedule = () => {
      editingIndex.value = -1
      editingSchedule.value = {
        hour: 8,
        minute: 0,
        duration_min: 5,
        enabled: true
      }
      showModal.value = true
    }

    const editSchedule = (index) => {
      editingIndex.value = index
      editingSchedule.value = { ...schedules.value[index] }
      showModal.value = true
    }

    const removeSchedule = (index) => {
      schedules.value.splice(index, 1)
    }

    const toggleSchedule = (index) => {
      schedules.value[index].enabled = !schedules.value[index].enabled
    }

    const closeModal = () => {
      showModal.value = false
    }

    const confirmEdit = () => {
      if (editingIndex.value === -1) {
        schedules.value.push({ ...editingSchedule.value })
      } else {
        schedules.value[editingIndex.value] = { ...editingSchedule.value }
      }
      closeModal()
    }

    const saveSchedules = async () => {
      saving.value = true
      try {
        const response = await api.updateSchedules(schedules.value)
        schedules.value = response.data
        originalSchedules.value = JSON.parse(JSON.stringify(response.data))
      } catch (error) {
        console.error('Failed to save schedules:', error)
      } finally {
        saving.value = false
      }
    }

    onMounted(() => {
      fetchSchedules()
      fetchTimezone()
    })

    return {
      schedules,
      loading,
      saving,
      hasChanges,
      showModal,
      editingIndex,
      editingSchedule,
      currentTimezone,
      selectedTimezone,
      savingTimezone,
      timezoneMessage,
      timezoneSuccess,
      formatTime,
      formatUtcOffset,
      addSchedule,
      editSchedule,
      removeSchedule,
      toggleSchedule,
      closeModal,
      confirmEdit,
      saveSchedules,
      saveTimezone
    }
  }
}
</script>
