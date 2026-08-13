set(MILESTONE "m5")

if("${MILESTONE}" STREQUAL "m3t1")
  set(SFILES main_m3t1.c delay.c coef.c filter.c)
  set(COMPS config esp_driver_gpio esp_timer lcd test)

elseif("${MILESTONE}" STREQUAL "m3t2")
  set(SFILES main_m3t2.c trigger.c tx.c hitLedTimer.c shot.c)
  set(COMPS config esp_driver_gpio esp_driver_ledc esp_timer lcd neo test)

elseif("${MILESTONE}" STREQUAL "m3t3")
  set(SFILES main_m3t3.c delay.c coef.c filter.c tx.c buffer.c detector.c)
  set(COMPS config esp_driver_gpio esp_driver_ledc esp_timer lcd test)

elseif("${MILESTONE}" STREQUAL "m4")
  set(SFILES main_m4.c delay.c coef.c filter.c trigger.c tx.c hitLedTimer.c shot.c buffer.c detector.c)
  set(COMPS config esp_driver_gpio esp_driver_ledc esp_timer lcd neo histogram test)

elseif("${MILESTONE}" STREQUAL "m5")
  set(SFILES main_m5.c delay.c coef.c filter.c trigger.c tx.c hitLedTimer.c shot.c buffer.c detector.c invincibilityTimer.c lockoutTimer.c game.c)
  set(COMPS config esp_driver_gpio esp_driver_ledc esp_timer lcd panel histogram neo sound c32k_16b)

elseif("${MILESTONE}" STREQUAL "m6")
  set(SFILES main_m6.c delay.c coef.c filter.c trigger.c tx.c hitLedTimer.c shot.c buffer.c detector.c invincibilityTimer.c lockoutTimer.c creative.c)
  set(COMPS config esp_driver_gpio esp_driver_ledc esp_timer lcd panel histogram neo net sound c32k_16b)

elseif("${MILESTONE}" STREQUAL "diag")
  set(SFILES main_diag.c delay.c coef.c filter.c tx.c hitLedTimer.c buffer.c)
  set(COMPS config esp_driver_gpio esp_driver_ledc esp_timer lcd histogram neo sound c32k_16b test)

endif()

idf_component_register(SRCS ${SFILES} INCLUDE_DIRS . PRIV_REQUIRES ${COMPS} esp_adc)
message(STATUS "MILESTONE=${MILESTONE}")

if("${MILESTONE}" STREQUAL "m3t2" AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/lib/libm3t2.a)
  add_prebuilt_library(m3t2 "lib/libm3t2.a" PRIV_REQUIRES esp_adc main)
  target_link_libraries(${COMPONENT_LIB} PRIVATE m3t2)
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/lib/librx.a)
  add_prebuilt_library(rx "lib/librx.a" PRIV_REQUIRES esp_adc main)
  target_link_libraries(${COMPONENT_LIB} PRIVATE rx)
endif()
