import { createApp } from 'vue'
import { createRouter, createWebHistory } from 'vue-router'
import App from './App.vue'
import './style.css'

// Import views
import Dashboard from './views/Dashboard.vue'
import Schedule from './views/Schedule.vue'
import History from './views/History.vue'

// Define routes
const routes = [
  { path: '/', name: 'Dashboard', component: Dashboard },
  { path: '/schedule', name: 'Schedule', component: Schedule },
  { path: '/history', name: 'History', component: History }
]

// Create router
const router = createRouter({
  history: createWebHistory(),
  routes
})

// Create and mount app
createApp(App)
  .use(router)
  .mount('#app')
