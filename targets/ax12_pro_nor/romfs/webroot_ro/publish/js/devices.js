(window.webpackJsonp=window.webpackJsonp||[]).push([["devices"],{4938:function(e,i,t){"use strict";t.r(i);var a=t("d2db"),n=t("7886"),s=t("710d"),r={mixins:[a.a],data(){return{ruleForm:{wifiTimeEn:!1,wifiTimeClose:[],wifiTimeDate:[]},rules:{wifiTimeClose:[this.checkTime],wifiTimeDate:[this.checkDate]},dateOptions:Object(s.e)(n.b)}},methods:{changeWifiTimeEn(e){this.$emit("change-enable",e)},checkDate(e){if(0===e.length)return _("Please select at least one day")},checkTime(e){if(e[0]===e[1]&&e[0])return _("The end time and the start time cannot be the same")},beforeSubmit(e){e.wifiTimeClose&&(e.wifiTimeClose=e.wifiTimeClose.join("-")),e.wifiTimeDate&&(e.wifiTimeDate=Object(s.i)(e.wifiTimeDate))}}},m=t("0b56"),o={components:{WifiTime:Object(m.a)(r,(function render(){var e=this,i=e._self._c;return i("v-form",{ref:e.name,attrs:{rules:e.rules}},[i("v-form-item",{attrs:{label:e._("WiFi Schedule"),prop:"wifiTimeEn"}},[i("v-switch",{on:{change:e.changeWifiTimeEn},model:{value:e.ruleForm.wifiTimeEn,callback:function(i){e.$set(e.ruleForm,"wifiTimeEn",i)},expression:"ruleForm.wifiTimeEn"}})],1),i("collapse-transition",[e.ruleForm.wifiTimeEn?i("div",[i("v-form-item",{attrs:{label:e._("Turn Off at"),prop:"wifiTimeClose"}},[i("v-timepicker",{attrs:{placeholder:e._("Start Time"),"end-placeholder":e._("End Time"),"is-range":""},model:{value:e.ruleForm.wifiTimeClose,callback:function(i){e.$set(e.ruleForm,"wifiTimeClose",i)},expression:"ruleForm.wifiTimeClose"}})],1),i("v-form-item",{attrs:{label:e._("Repeat"),prop:"wifiTimeDate",required:!1}},[i("v-checkbox-group",{attrs:{"is-select-all":"","select-text":e._("Every Day"),options:e.dateOptions},model:{value:e.ruleForm.wifiTimeDate,callback:function(i){e.$set(e.ruleForm,"wifiTimeDate",i)},expression:"ruleForm.wifiTimeDate"}})],1)],1):e._e()])],1)}),[],!1,null,null,null).exports,appQrcode:t("f1fe").a},data:()=>({pageTips:_("Disable the WiFi network in a specified period, and enable at other times."),wifiTime:{},title:_("How to connect to the WiFi network during WiFi-disabling period?"),tips:[_("Method 1: Use the Tenda WiFi app with your account and enable/disable the WiFi network anytime, anywhere."),_("Method 2: Use an Ethernet cable to connect your computer to the router, visit tendawifi.com to log in to the web UI, and enable the WiFi network manually.")],isShowSubmit:!1,isSyncInternetTime:!1}),created(){this.getData()},methods:{getData(){this.$getData({modules:"wifiTime,systemTime"}).then(e=>{e.wifiTime.wifiTimeClose=e.wifiTime.wifiTimeClose.split("-"),e.wifiTime.wifiTimeDate=Object(s.l)(e.wifiTime.wifiTimeDate),this.wifiTime=Object(s.e)(e.wifiTime),this.isShowSubmit=e.wifiTime.wifiTimeEn,this.isSyncInternetTime=e.systemTime.isSyncInternetTime})},submit(e){this.wifiTime.wifiTimeEn=e,e||this.$nextTick((function(){this.$refs.page.submitForm()})),this.isShowSubmit=e}}},l=Object(m.a)(o,(function render(){var e=this,i=e._self._c;return i("div",[i("v-page",{ref:"page",attrs:{"show-footer":e.isShowSubmit},on:{init:e.getData}},[i("v-page-title",{attrs:{title:e._("WiFi Schedule"),tips:e.pageTips}}),i("wifi-time",{attrs:{name:"wifiTime",formData:e.wifiTime},on:{"change-enable":e.submit}})],1),e.isSyncInternetTime?i("div",{staticClass:"error-text text-center"},[e._v(" "+e._s(e._("The Schedule Disable time takes effect based on the system time"))+" ")]):e._e(),i("app-qrcode",{attrs:{title:e.title,tips:e.tips}})],1)}),[],!1,null,null,null);i.default=l.exports}}]);