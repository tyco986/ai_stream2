import { createApp } from 'vue'
import { createPinia } from 'pinia'
import ElementPlus from 'element-plus'
import VueKonva from 'vue-konva'
import 'element-plus/dist/index.css'

import App from './App.vue'
import { router } from './router'
import { i18n } from './shared/i18n'

const app = createApp(App)
app.use(createPinia())
app.use(router)
app.use(i18n)
app.use(ElementPlus)
app.use(VueKonva)
app.mount('#app')
