idf_component_register(
SRCS
  bcFire.c
  clipEmpty.c
  gameBoyStartup.c
  gameOver.c
  ouch.c
  pacmanDeath.c
  powerUp.c
  screamAndDie.c
INCLUDE_DIRS
  .
)
# PRIV_REQUIRES driver
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
