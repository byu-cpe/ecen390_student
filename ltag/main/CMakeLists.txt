set(MILESTONE "m5")

if("${MILESTONE}" STREQUAL "m3t1")
  set(SFILES main_m3t1.c delay.c coef.c filter.c)
  set(COMPS config driver esp_timer lcd test)

elseif("${MILESTONE}" STREQUAL "m3t2")
  set(SFILES main_m3t2.c trigger.c tx.c hitLedTimer.c)
  set(COMPS config driver esp_timer lcd neo test esp_adc)

elseif("${MILESTONE}" STREQUAL "m3t3")
  set(SFILES main_m3t3.c buffer.c delay.c coef.c filter.c detector.c shot.c tx.c)
  set(COMPS config driver esp_timer lcd test rx esp_adc)

elseif("${MILESTONE}" STREQUAL "m4")
  set(SFILES main_m4.c tx.c buffer.c delay.c coef.c filter.c detector.c trigger.c hitLedTimer.c shot.c)
  set(COMPS config driver esp_timer lcd neo histogram test rx esp_adc)

elseif("${MILESTONE}" STREQUAL "m5")
  set(SFILES main_m5.c tx.c buffer.c delay.c coef.c filter.c detector.c game.c trigger.c hitLedTimer.c invincibilityTimer.c lockoutTimer.c shot.c)
  set(COMPS config driver esp_timer esp_adc lcd rx panel histogram neo sound c32k_16b)

elseif("${MILESTONE}" STREQUAL "m6")
  set(SFILES main_m6.c tx.c buffer.c delay.c coef.c filter.c detector.c creative.c trigger.c hitLedTimer.c invincibilityTimer.c lockoutTimer.c shot.c)
  set(COMPS config driver esp_timer esp_adc lcd rx panel histogram neo net sound c32k_16b)

elseif("${MILESTONE}" STREQUAL "diag")
  set(SFILES main_diag.c buffer.c delay.c coef.c filter.c tx.c hitLedTimer.c)
  set(COMPS config driver esp_timer esp_adc lcd histogram neo sound c32k_16b test rx)

endif()

idf_component_register(SRCS ${SFILES} INCLUDE_DIRS . PRIV_REQUIRES ${COMPS})
message(STATUS "MILESTONE=${MILESTONE}")

if("${MILESTONE}" STREQUAL "m3t2" AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/test/libm3t2.a)
  add_prebuilt_library(m3t2 "test/libm3t2.a" PRIV_REQUIRES esp_adc)
  target_link_libraries(${COMPONENT_LIB} PRIVATE m3t2)
endif()
