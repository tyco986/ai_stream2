import { reactive } from "vue";
import type { FormRules } from "element-plus";
import { $t, transformI18n } from "@/plugins/i18n";

const loginRules = reactive<FormRules>({
  password: [
    {
      required: true,
      message: transformI18n($t("login.purePassWordReg")),
      trigger: "blur"
    }
  ]
});

export { loginRules };
