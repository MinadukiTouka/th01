static const shiftjis_kanji_t SCOREDAT_ROUTE_NONE   = kanji_swap('無');
static const shiftjis_kanji_t SCOREDAT_ROUTE_CLEAR  = kanji_swap('完');
static const shiftjis_kanji_t SCOREDAT_ROUTE_SHRINE = kanji_swap('社');
static const shiftjis_kanji_t SCOREDAT_ROUTE_MAKAI  = kanji_swap('魔');
static const shiftjis_kanji_t SCOREDAT_ROUTE_JIGOKU = kanji_swap('地');

// Will be name-encoded, and therefore modified in the .data section!
#define scoredat_name_default	"東方★靈異伝　　"

#define SCOREDAT_ERROR_NOT_FOUND	"ファイルがみつかんない"
#define SCOREDAT_ERROR_INVALID  	"ファイルが違う！"
