#include <bcm2835.h>
#include <inttypes.h>

using namespace std;

typedef enum {
    HAB_SPI_CSA,
    HAB_SPI_CSB,
    HAB_SPI_CSC,
    HAB_SPI_CSD
} HAB_SPI_CS;

class HABSPI {
        HAB_SPI_CS _cs;
        uint8_t _auxA, _auxB;
        
        void _setup(void);
  public:
        HABSPI(HAP_SPI_CS aCS, uint8_t A, uint8_t B);
        void lower_cs(void);
        void raise_cs(void);
}