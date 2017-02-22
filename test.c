#include <math.h>
#include <stdio.h>

static const float EARTH_RADIUS = 6378.137; // Radius of earth in KM

float distance(float lat1, float lon1, float lat2, float lon2){  // generally used geo measurement function
    float R = 6378.137; // Radius of earth in KM
    float dLat = lat2 * M_PI / 180 - lat1 * M_PI / 180;
    float dLon = lon2 * M_PI / 180 - lon1 * M_PI / 180;
    float a = sinf(dLat/2) * sinf(dLat/2) +
    cosf(lat1 * M_PI / 180) * cosf(lat2 * M_PI / 180) *
    sinf(dLon/2) * sinf(dLon/2);
    float c = 2 * atan2(sqrtf(a), sqrtf(1-a));
    float d = R * c;
    return d * 1000; // meters
}

void to_gps(float x, float y, float latitude, float longitude, float *lat, float *lon){
	*lat = latitude  + (y / EARTH_RADIUS * 0.001) * (180 / M_PI);
	*lon = longitude + (x / EARTH_RADIUS * 0.001) * (180 / M_PI) / cosf(latitude * M_PI/180);
}

int main(void){
	float lat1 = 59.329323;
	float lon1 = 18.068581;
	float lat2 = 59.195363;
	float lon2 = 17.625689;
	printf("distance from a to b %f\n", distance(lat1, lon1, lat2, lon2));
	float dlat = 0, dlon = 0;
	to_gps(29000, 0, lat1, lon1, &dlat, &dlon);
	printf("distance nr 2: %f\n", distance(lat1, lon1, dlat, dlon));
	return 0;
}
