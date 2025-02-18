set(MILESTONE "m3t1")

if("${MILESTONE}" STREQUAL "m3t1")
  set(SFILES main_m3t1.c delay.c coef.c filter.c)
  set(COMPS config driver esp_timer lcd test)

elseif("${MILESTONE}" STREQUAL "m3t2")
  set(SFILES main_m3t2.c trigger.c tx.c hitLedTimer.c)
  set(COMPS config driver esp_timer lcd neo test)

elseif("${MILESTONE}" STREQUAL "m3t3")
  set(SFILES main_m3t3.c delay.c coef.c filter.c tx.c hitLedTimer.c buffer.c rx.c)
  set(COMPS config driver esp_timer esp_adc lcd histogram neo sound c32k_16b test)

elseif("${MILESTONE}" STREQUAL "m5")
  set(SFILES main_m5.c tx.c rx.c buffer.c delay.c filter.c coef.c detector.c game.c trigger.c hitLedTimer.c invincibilityTimer.c lockoutTimer.c shot.c)
  set(COMPS config driver esp_timer esp_adc lcd histogram neo sound c32k_16b)

endif()
idf_component_register(SRCS ${SFILES} INCLUDE_DIRS . PRIV_REQUIRES ${COMPS})
# target_compile_options(${COMPONENT_LIB} PRIVATE -DDIAG) # use with m3t3
message(STATUS "MILESTONE=${MILESTONE}")

if("${MILESTONE}" STREQUAL "m3t2" AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/test/libm3t2.a)
  add_prebuilt_library(m3t2 "test/libm3t2.a" PRIV_REQUIRES esp_adc)
  target_link_libraries(${COMPONENT_LIB} PRIVATE m3t2)
endif()
