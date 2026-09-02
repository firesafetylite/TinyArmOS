// TinyArmOS firmware-backed HTTPS updater.
// Uses UEFI HTTP/TLS services supplied by the VM firmware.

#define UPDATE_MANIFEST_URL "https://firesafetylite.github.io/TinyArmOS/TinyArmOS-update.txt"
#define UPDATE_MAX_MANIFEST_BYTES 4096U
#define UPDATE_MAX_IMAGE_BYTES (8U * 1024U * 1024U)
#define UPDATE_MAX_URL_BYTES 4096U
#define UPDATE_MAX_REDIRECTS 8U
#define UPDATE_HTTP_TIMEOUT_SECONDS 30U
#define UPDATE_DHCP_TIMEOUT_SECONDS 10U
#define UPDATE_LOCATE_ALL_HANDLES 0U

typedef struct EFI_SERVICE_BINDING_PROTOCOL EFI_SERVICE_BINDING_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_SERVICE_BINDING_CREATE_CHILD)(EFI_SERVICE_BINDING_PROTOCOL *, EFI_HANDLE *);
typedef EFI_STATUS (EFIAPI *EFI_SERVICE_BINDING_DESTROY_CHILD)(EFI_SERVICE_BINDING_PROTOCOL *, EFI_HANDLE);
struct EFI_SERVICE_BINDING_PROTOCOL {
    EFI_SERVICE_BINDING_CREATE_CHILD CreateChild;
    EFI_SERVICE_BINDING_DESTROY_CHILD DestroyChild;
};

typedef struct {
    UINT8 Addr[4];
} EFI_IPv4_ADDRESS;

typedef struct UPDATE_IP4_CONFIG2_PROTOCOL UPDATE_IP4_CONFIG2_PROTOCOL;
typedef EFI_STATUS (EFIAPI *UPDATE_IP4_CONFIG2_SET_DATA)(UPDATE_IP4_CONFIG2_PROTOCOL *, UINT32, UINTN, void *);
typedef EFI_STATUS (EFIAPI *UPDATE_IP4_CONFIG2_GET_DATA)(UPDATE_IP4_CONFIG2_PROTOCOL *, UINT32, UINTN *, void *);
struct UPDATE_IP4_CONFIG2_PROTOCOL {
    UPDATE_IP4_CONFIG2_SET_DATA SetData;
    UPDATE_IP4_CONFIG2_GET_DATA GetData;
    void *RegisterDataNotify;
    void *UnregisterDataNotify;
};

#define UPDATE_IP4_CONFIG2_POLICY 1U
#define UPDATE_IP4_CONFIG2_MANUAL_ADDRESS 2U
#define UPDATE_IP4_CONFIG2_GATEWAY 3U
#define UPDATE_IP4_CONFIG2_DNS_SERVER 4U
#define UPDATE_IP4_POLICY_STATIC 0U
#define UPDATE_IP4_POLICY_DHCP 1U

typedef struct {
    EFI_IPv4_ADDRESS Address;
    EFI_IPv4_ADDRESS SubnetMask;
} UPDATE_IP4_MANUAL_ADDRESS;

typedef enum {
    HttpVersion10,
    HttpVersion11,
    HttpVersionUnsupported
} EFI_HTTP_VERSION;

typedef enum {
    HttpMethodGet,
    HttpMethodPost,
    HttpMethodPatch,
    HttpMethodOptions,
    HttpMethodConnect,
    HttpMethodHead,
    HttpMethodPut,
    HttpMethodDelete,
    HttpMethodTrace,
    HttpMethodMax
} EFI_HTTP_METHOD;

typedef enum {
    HTTP_STATUS_UNSUPPORTED_STATUS = 0,
    HTTP_STATUS_100_CONTINUE,
    HTTP_STATUS_101_SWITCHING_PROTOCOLS,
    HTTP_STATUS_200_OK,
    HTTP_STATUS_201_CREATED,
    HTTP_STATUS_202_ACCEPTED,
    HTTP_STATUS_203_NON_AUTHORITATIVE_INFORMATION,
    HTTP_STATUS_204_NO_CONTENT,
    HTTP_STATUS_205_RESET_CONTENT,
    HTTP_STATUS_206_PARTIAL_CONTENT,
    HTTP_STATUS_300_MULTIPLE_CHOICES,
    HTTP_STATUS_301_MOVED_PERMANENTLY,
    HTTP_STATUS_302_FOUND,
    HTTP_STATUS_303_SEE_OTHER,
    HTTP_STATUS_304_NOT_MODIFIED,
    HTTP_STATUS_305_USE_PROXY,
    HTTP_STATUS_307_TEMPORARY_REDIRECT,
    HTTP_STATUS_400_BAD_REQUEST,
    HTTP_STATUS_401_UNAUTHORIZED,
    HTTP_STATUS_402_PAYMENT_REQUIRED,
    HTTP_STATUS_403_FORBIDDEN,
    HTTP_STATUS_404_NOT_FOUND,
    HTTP_STATUS_405_METHOD_NOT_ALLOWED,
    HTTP_STATUS_406_NOT_ACCEPTABLE,
    HTTP_STATUS_407_PROXY_AUTHENTICATION_REQUIRED,
    HTTP_STATUS_408_REQUEST_TIME_OUT,
    HTTP_STATUS_409_CONFLICT,
    HTTP_STATUS_410_GONE,
    HTTP_STATUS_411_LENGTH_REQUIRED,
    HTTP_STATUS_412_PRECONDITION_FAILED,
    HTTP_STATUS_413_REQUEST_ENTITY_TOO_LARGE,
    HTTP_STATUS_414_REQUEST_URI_TOO_LARGE,
    HTTP_STATUS_415_UNSUPPORTED_MEDIA_TYPE,
    HTTP_STATUS_416_REQUESTED_RANGE_NOT_SATISFIED,
    HTTP_STATUS_417_EXPECTATION_FAILED,
    HTTP_STATUS_500_INTERNAL_SERVER_ERROR,
    HTTP_STATUS_501_NOT_IMPLEMENTED,
    HTTP_STATUS_502_BAD_GATEWAY,
    HTTP_STATUS_503_SERVICE_UNAVAILABLE,
    HTTP_STATUS_504_GATEWAY_TIME_OUT,
    HTTP_STATUS_505_HTTP_VERSION_NOT_SUPPORTED,
    HTTP_STATUS_308_PERMANENT_REDIRECT,
    HTTP_STATUS_429_TOO_MANY_REQUESTS
} EFI_HTTP_STATUS_CODE;

typedef struct {
    UINT8 UseDefaultAddress;
    EFI_IPv4_ADDRESS LocalAddress;
    EFI_IPv4_ADDRESS LocalSubnet;
    UINT16 LocalPort;
} EFI_HTTPv4_ACCESS_POINT;

typedef struct {
    EFI_HTTP_VERSION HttpVersion;
    UINT32 TimeOutMillisec;
    UINT8 LocalAddressIsIPv6;
    union {
        EFI_HTTPv4_ACCESS_POINT *IPv4Node;
        void *IPv6Node;
    } AccessPoint;
} EFI_HTTP_CONFIG_DATA;

typedef struct {
    EFI_HTTP_METHOD Method;
    CHAR16 *Url;
} EFI_HTTP_REQUEST_DATA;

typedef struct {
    EFI_HTTP_STATUS_CODE StatusCode;
} EFI_HTTP_RESPONSE_DATA;

typedef struct {
    char *FieldName;
    char *FieldValue;
} EFI_HTTP_HEADER;

typedef struct {
    union {
        EFI_HTTP_REQUEST_DATA *Request;
        EFI_HTTP_RESPONSE_DATA *Response;
    } Data;
    UINTN HeaderCount;
    EFI_HTTP_HEADER *Headers;
    UINTN BodyLength;
    void *Body;
} EFI_HTTP_MESSAGE;

typedef struct {
    EFI_EVENT Event;
    EFI_STATUS Status;
    EFI_HTTP_MESSAGE *Message;
} EFI_HTTP_TOKEN;

typedef struct EFI_HTTP_PROTOCOL EFI_HTTP_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_HTTP_GET_MODE_DATA)(EFI_HTTP_PROTOCOL *, EFI_HTTP_CONFIG_DATA *);
typedef EFI_STATUS (EFIAPI *EFI_HTTP_CONFIGURE)(EFI_HTTP_PROTOCOL *, EFI_HTTP_CONFIG_DATA *);
typedef EFI_STATUS (EFIAPI *EFI_HTTP_REQUEST)(EFI_HTTP_PROTOCOL *, EFI_HTTP_TOKEN *);
typedef EFI_STATUS (EFIAPI *EFI_HTTP_CANCEL)(EFI_HTTP_PROTOCOL *, EFI_HTTP_TOKEN *);
typedef EFI_STATUS (EFIAPI *EFI_HTTP_RESPONSE)(EFI_HTTP_PROTOCOL *, EFI_HTTP_TOKEN *);
typedef EFI_STATUS (EFIAPI *EFI_HTTP_POLL)(EFI_HTTP_PROTOCOL *);
struct EFI_HTTP_PROTOCOL {
    EFI_HTTP_GET_MODE_DATA GetModeData;
    EFI_HTTP_CONFIGURE Configure;
    EFI_HTTP_REQUEST Request;
    EFI_HTTP_CANCEL Cancel;
    EFI_HTTP_RESPONSE Response;
    EFI_HTTP_POLL Poll;
};

typedef struct {
    EFI_SERVICE_BINDING_PROTOCOL *binding;
    EFI_HANDLE child;
    EFI_HTTP_PROTOCOL *http;
} UPDATE_HTTP_SESSION;

typedef struct {
    char version[32];
    UINTN size;
    UINT8 digest[32];
    char url[UPDATE_MAX_URL_BYTES];
} UPDATE_MANIFEST;

typedef struct {
    UINT32 state[8];
    UINT64 bitLength;
    UINT8 block[64];
    UINTN blockLength;
} UPDATE_SHA256;

static EFI_GUID gUpdateIp4Config2ProtocolGuid = {
    0x5b446ed1, 0xe30b, 0x4faa, {0x87,0x1a,0x36,0x54,0xec,0xa3,0x60,0x80}
};
static EFI_GUID gUpdateHttpServiceBindingGuid = {
    0xbdc8e6af, 0xd9bc, 0x4379, {0xa7,0x2a,0xe0,0xc4,0xe7,0x5d,0xae,0x1c}
};
static EFI_GUID gUpdateHttpProtocolGuid = {
    0x7a59b29b, 0x910b, 0x4171, {0x82,0x42,0xa8,0x5a,0x0d,0xf2,0x5b,0x5b}
};

static const CHAR16 gUpdateBootPath[] = {
    '\\','E','F','I','\\','B','O','O','T','\\','B','O','O','T','A','A','6','4','.','E','F','I',0
};
static const CHAR16 gUpdateBackupPath[] = {
    '\\','E','F','I','\\','B','O','O','T','\\','B','O','O','T','A','A','6','4','.','B','A','K',0
};
static const CHAR16 gUpdateStagePath[] = {
    '\\','E','F','I','\\','B','O','O','T','\\','B','O','O','T','A','A','6','4','.','N','E','W',0
};

static UINT32 update_rotr32(UINT32 value, UINT32 count) {
    return (value >> count) | (value << (32U - count));
}

static void update_sha256_transform(UPDATE_SHA256 *context, const UINT8 *block) {
    static const UINT32 constants[64] = {
        0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
        0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
        0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
        0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
        0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
        0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
        0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
        0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U
    };
    UINT32 words[64];
    UINT32 a;
    UINT32 b;
    UINT32 c;
    UINT32 d;
    UINT32 e;
    UINT32 f;
    UINT32 g;
    UINT32 h;
    UINT32 index;
    for (index = 0; index < 16U; index++) {
        UINTN offset = (UINTN)index * 4U;
        words[index] = ((UINT32)block[offset] << 24) |
                       ((UINT32)block[offset + 1U] << 16) |
                       ((UINT32)block[offset + 2U] << 8) |
                       (UINT32)block[offset + 3U];
    }
    for (index = 16U; index < 64U; index++) {
        UINT32 s0 = update_rotr32(words[index - 15U], 7U) ^
                    update_rotr32(words[index - 15U], 18U) ^
                    (words[index - 15U] >> 3U);
        UINT32 s1 = update_rotr32(words[index - 2U], 17U) ^
                    update_rotr32(words[index - 2U], 19U) ^
                    (words[index - 2U] >> 10U);
        words[index] = words[index - 16U] + s0 + words[index - 7U] + s1;
    }
    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    f = context->state[5];
    g = context->state[6];
    h = context->state[7];
    for (index = 0; index < 64U; index++) {
        UINT32 sum1 = update_rotr32(e, 6U) ^ update_rotr32(e, 11U) ^ update_rotr32(e, 25U);
        UINT32 choose = (e & f) ^ ((~e) & g);
        UINT32 temp1 = h + sum1 + choose + constants[index] + words[index];
        UINT32 sum0 = update_rotr32(a, 2U) ^ update_rotr32(a, 13U) ^ update_rotr32(a, 22U);
        UINT32 majority = (a & b) ^ (a & c) ^ (b & c);
        UINT32 temp2 = sum0 + majority;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
    context->state[5] += f;
    context->state[6] += g;
    context->state[7] += h;
}

static void update_sha256_init(UPDATE_SHA256 *context) {
    context->state[0] = 0x6a09e667U;
    context->state[1] = 0xbb67ae85U;
    context->state[2] = 0x3c6ef372U;
    context->state[3] = 0xa54ff53aU;
    context->state[4] = 0x510e527fU;
    context->state[5] = 0x9b05688cU;
    context->state[6] = 0x1f83d9abU;
    context->state[7] = 0x5be0cd19U;
    context->bitLength = 0;
    context->blockLength = 0;
}

static void update_sha256_add(UPDATE_SHA256 *context, const UINT8 *data, UINTN bytes) {
    UINTN index;
    for (index = 0; index < bytes; index++) {
        context->block[context->blockLength++] = data[index];
        if (context->blockLength == 64U) {
            update_sha256_transform(context, context->block);
            context->bitLength += 512U;
            context->blockLength = 0;
        }
    }
}

static void update_sha256_finish(UPDATE_SHA256 *context, UINT8 digest[32]) {
    UINTN index = context->blockLength;
    UINT64 totalBits;
    context->block[index++] = 0x80U;
    if (index > 56U) {
        while (index < 64U) context->block[index++] = 0;
        update_sha256_transform(context, context->block);
        index = 0;
    }
    while (index < 56U) context->block[index++] = 0;
    totalBits = context->bitLength + (UINT64)context->blockLength * 8U;
    for (index = 0; index < 8U; index++) {
        context->block[63U - index] = (UINT8)(totalBits >> (index * 8U));
    }
    update_sha256_transform(context, context->block);
    for (index = 0; index < 8U; index++) {
        digest[index * 4U] = (UINT8)(context->state[index] >> 24);
        digest[index * 4U + 1U] = (UINT8)(context->state[index] >> 16);
        digest[index * 4U + 2U] = (UINT8)(context->state[index] >> 8);
        digest[index * 4U + 3U] = (UINT8)context->state[index];
    }
}

static void update_sha256_bytes(const UINT8 *data, UINTN bytes, UINT8 digest[32]) {
    UPDATE_SHA256 context;
    update_sha256_init(&context);
    update_sha256_add(&context, data, bytes);
    update_sha256_finish(&context, digest);
}

static char update_ascii_lower(char value) {
    if (value >= 'A' && value <= 'Z') return (char)(value + ('a' - 'A'));
    return value;
}

static int update_ascii_equal_ci(const char *left, const char *right) {
    while (*left && *right && update_ascii_lower(*left) == update_ascii_lower(*right)) {
        left++;
        right++;
    }
    return update_ascii_lower(*left) == update_ascii_lower(*right);
}

static int update_copy(char *destination, const char *source, UINTN capacity) {
    UINTN length = string_length(source);
    if (!capacity || length + 1U > capacity) return 0;
    memory_copy(destination, source, length + 1U);
    return 1;
}

static int update_append(char *destination, const char *source, UINTN capacity) {
    UINTN used = string_length(destination);
    if (used >= capacity) return 0;
    return update_copy(destination + used, source, capacity - used);
}

static int update_ascii_to_wide(const char *source, CHAR16 *destination, UINTN capacity) {
    UINTN index = 0;
    while (source[index]) {
        if (index + 1U >= capacity || (UINT8)source[index] >= 128U) return 0;
        destination[index] = (CHAR16)(UINT8)source[index];
        index++;
    }
    destination[index] = 0;
    return 1;
}

static int update_https_url(const char *url) {
    return starts_with(url, "https://") && string_length(url) < UPDATE_MAX_URL_BYTES;
}

static int update_url_host(const char *url, char *host, UINTN capacity) {
    const char *cursor;
    UINTN used = 0;
    if (!update_https_url(url)) return 0;
    cursor = url + 8;
    while (*cursor && *cursor != '/' && *cursor != '?' && *cursor != '#') {
        if (used + 1U >= capacity) return 0;
        host[used++] = *cursor++;
    }
    if (!used) return 0;
    host[used] = 0;
    return 1;
}

static int update_resolve_redirect(const char *current, const char *location, char *output, UINTN capacity) {
    const char *hostEnd;
    UINTN originLength;
    if (starts_with(location, "https://")) return update_copy(output, location, capacity);
    if (location[0] != '/' || !update_https_url(current)) return 0;
    hostEnd = current + 8;
    while (*hostEnd && *hostEnd != '/') hostEnd++;
    originLength = (UINTN)(hostEnd - current);
    if (originLength + string_length(location) + 1U > capacity) return 0;
    memory_copy(output, current, originLength);
    output[originLength] = 0;
    return update_append(output, location, capacity);
}

static UINTN update_decimal(const char *text, int *okay) {
    UINTN value = 0;
    *okay = 0;
    if (!*text) return 0;
    while (*text) {
        UINTN digit;
        if (*text < '0' || *text > '9') return 0;
        digit = (UINTN)(*text - '0');
        if (value > (UPDATE_MAX_IMAGE_BYTES - digit) / 10U) return 0;
        value = value * 10U + digit;
        text++;
    }
    *okay = 1;
    return value;
}

static int update_hex_digit(char value, UINT8 *digit) {
    if (value >= '0' && value <= '9') {
        *digit = (UINT8)(value - '0');
        return 1;
    }
    value = update_ascii_lower(value);
    if (value >= 'a' && value <= 'f') {
        *digit = (UINT8)(value - 'a' + 10);
        return 1;
    }
    return 0;
}

static int update_parse_digest(const char *text, UINT8 digest[32]) {
    UINTN index;
    if (string_length(text) != 64U) return 0;
    for (index = 0; index < 32U; index++) {
        UINT8 high;
        UINT8 low;
        if (!update_hex_digit(text[index * 2U], &high) ||
            !update_hex_digit(text[index * 2U + 1U], &low)) return 0;
        digest[index] = (UINT8)((high << 4) | low);
    }
    return 1;
}

static int update_digest_equal(const UINT8 left[32], const UINT8 right[32]) {
    UINT8 difference = 0;
    UINTN index;
    for (index = 0; index < 32U; index++) difference |= (UINT8)(left[index] ^ right[index]);
    return difference == 0;
}

static void update_free_headers(EFI_HTTP_MESSAGE *message) {
    UINTN index;
    if (!message->Headers) return;
    for (index = 0; index < message->HeaderCount; index++) {
        if (message->Headers[index].FieldName) gST->BootServices->FreePool(message->Headers[index].FieldName);
        if (message->Headers[index].FieldValue) gST->BootServices->FreePool(message->Headers[index].FieldValue);
    }
    gST->BootServices->FreePool(message->Headers);
    message->Headers = (EFI_HTTP_HEADER *)0;
    message->HeaderCount = 0;
}

static const char *update_find_header(const EFI_HTTP_MESSAGE *message, const char *name) {
    UINTN index;
    for (index = 0; index < message->HeaderCount; index++) {
        if (message->Headers[index].FieldName && message->Headers[index].FieldValue &&
            update_ascii_equal_ci(message->Headers[index].FieldName, name)) {
            return message->Headers[index].FieldValue;
        }
    }
    return (const char *)0;
}

static void EFIAPI update_http_notify(EFI_EVENT event, void *context) {
    (void)event;
    *(volatile UINT8 *)context = 1;
}

static EFI_STATUS update_http_wait(UPDATE_HTTP_SESSION *session, EFI_HTTP_TOKEN *token, volatile UINT8 *done) {
    UINT64 deadline = timer_count() + gTimerHz * UPDATE_HTTP_TIMEOUT_SECONDS;
    while (!*done && timer_count() < deadline) {
        EFI_STATUS pollStatus = session->http->Poll(session->http);
        if (pollStatus != EFI_SUCCESS && pollStatus != EFI_NOT_READY) __asm__ volatile("yield");
    }
    if (!*done) {
        session->http->Cancel(session->http, token);
        return EFI_TIMEOUT;
    }
    return token->Status;
}

static void update_http_close(UPDATE_HTTP_SESSION *session) {
    if (session->http) session->http->Configure(session->http, (EFI_HTTP_CONFIG_DATA *)0);
    if (session->binding && session->child) session->binding->DestroyChild(session->binding, session->child);
    session->binding = (EFI_SERVICE_BINDING_PROTOCOL *)0;
    session->child = (EFI_HANDLE)0;
    session->http = (EFI_HTTP_PROTOCOL *)0;
}

static void update_connect_controllers(void) {
    EFI_HANDLE *handles = (EFI_HANDLE *)0;
    UINTN handleCount = 0;
    UINTN index;
    EFI_STATUS status;
    if (!gST->BootServices->LocateHandleBuffer || !gST->BootServices->ConnectController) return;
    status = gST->BootServices->LocateHandleBuffer(UPDATE_LOCATE_ALL_HANDLES, (EFI_GUID *)0,
        (void *)0, &handleCount, &handles);
    if (status != EFI_SUCCESS || !handles) return;
    for (index = 0; index < handleCount; index++) {
        gST->BootServices->ConnectController(handles[index], (EFI_HANDLE *)0, (void *)0, 1);
    }
    gST->BootServices->FreePool(handles);
}

static EFI_STATUS update_report_firmware_error(const char *stage, EFI_STATUS status) {
    print("Firmware network ");
    print(stage);
    print(" failed (UEFI status ");
    print_hex(status);
    print(").\n");
    return status;
}

static EFI_STATUS update_prepare_ipv4(void) {
    UPDATE_IP4_CONFIG2_PROTOCOL *config = (UPDATE_IP4_CONFIG2_PROTOCOL *)0;
    UPDATE_IP4_MANUAL_ADDRESS manual;
    EFI_IPv4_ADDRESS gateway = {{10, 0, 2, 2}};
    EFI_IPv4_ADDRESS dns = {{10, 0, 2, 3}};
    UINT32 policy = UPDATE_IP4_POLICY_DHCP;
    UINTN size;
    UINTN attempt;
    EFI_STATUS status;
    status = gST->BootServices->LocateProtocol(&gUpdateIp4Config2ProtocolGuid, (void *)0,
        (void **)&config);
    if (status != EFI_SUCCESS || !config) return status == EFI_SUCCESS ? EFI_NOT_FOUND : status;
    size = 0;
    status = config->GetData(config, UPDATE_IP4_CONFIG2_GATEWAY, &size, (void *)0);
    if (status == EFI_BUFFER_TOO_SMALL && size >= sizeof(EFI_IPv4_ADDRESS)) return EFI_SUCCESS;
    size = sizeof(policy);
    status = config->GetData(config, UPDATE_IP4_CONFIG2_POLICY, &size, &policy);
    if (status != EFI_SUCCESS || policy != UPDATE_IP4_POLICY_DHCP) {
        policy = UPDATE_IP4_POLICY_DHCP;
        status = config->SetData(config, UPDATE_IP4_CONFIG2_POLICY, sizeof(policy), &policy);
        if (status != EFI_SUCCESS && status != EFI_NOT_READY && status != EFI_ABORTED)
            return update_report_firmware_error("DHCP policy", status);
    }
    for (attempt = 0; attempt < UPDATE_DHCP_TIMEOUT_SECONDS * 5U; attempt++) {
        size = 0;
        status = config->GetData(config, UPDATE_IP4_CONFIG2_GATEWAY, &size, (void *)0);
        if (status == EFI_BUFFER_TOO_SMALL && size >= sizeof(EFI_IPv4_ADDRESS)) return EFI_SUCCESS;
        if (status != EFI_NOT_READY && status != EFI_NOT_FOUND && status != EFI_BUFFER_TOO_SMALL) return status;
        if (gST->BootServices->Stall) gST->BootServices->Stall(200000U);
    }

    /* UTM's Shared Network is QEMU user networking. Fall back to its stable
       addressing when a firmware DHCP client cannot obtain a lease. */
    print("Firmware DHCP timed out; trying UTM Shared Network defaults.\n");
    policy = UPDATE_IP4_POLICY_STATIC;
    status = config->SetData(config, UPDATE_IP4_CONFIG2_POLICY, sizeof(policy), &policy);
    if (status != EFI_SUCCESS && status != EFI_NOT_READY && status != EFI_ABORTED)
        return update_report_firmware_error("static policy", status);
    memory_zero(&manual, sizeof(manual));
    manual.Address.Addr[0] = 10; manual.Address.Addr[1] = 0;
    manual.Address.Addr[2] = 2; manual.Address.Addr[3] = 15;
    manual.SubnetMask.Addr[0] = 255; manual.SubnetMask.Addr[1] = 255;
    manual.SubnetMask.Addr[2] = 255; manual.SubnetMask.Addr[3] = 0;
    status = config->SetData(config, UPDATE_IP4_CONFIG2_MANUAL_ADDRESS,
        sizeof(manual), &manual);
    if (status != EFI_SUCCESS && status != EFI_NOT_READY && status != EFI_ABORTED)
        return update_report_firmware_error("static address", status);
    for (attempt = 0; attempt < 25U; attempt++) {
        size = 0;
        status = config->GetData(config, UPDATE_IP4_CONFIG2_MANUAL_ADDRESS, &size, (void *)0);
        if (status == EFI_BUFFER_TOO_SMALL && size >= sizeof(manual)) break;
        if (status != EFI_NOT_READY && status != EFI_NOT_FOUND && status != EFI_BUFFER_TOO_SMALL) return status;
        if (gST->BootServices->Stall) gST->BootServices->Stall(200000U);
    }
    if (attempt == 25U) return update_report_firmware_error("static address mapping", EFI_NO_MAPPING);
    status = config->SetData(config, UPDATE_IP4_CONFIG2_GATEWAY, sizeof(gateway), &gateway);
    if (status != EFI_SUCCESS && status != EFI_ABORTED)
        return update_report_firmware_error("static gateway", status);
    status = config->SetData(config, UPDATE_IP4_CONFIG2_DNS_SERVER, sizeof(dns), &dns);
    if (status != EFI_SUCCESS && status != EFI_ABORTED)
        return update_report_firmware_error("static DNS", status);
    return EFI_SUCCESS;
}

static EFI_STATUS update_http_open(UPDATE_HTTP_SESSION *session) {
    EFI_HTTPv4_ACCESS_POINT access;
    EFI_HTTP_CONFIG_DATA config;
    EFI_STATUS status;
    UINTN attempt;
    memory_zero(session, sizeof(*session));
    update_connect_controllers();
    status = update_prepare_ipv4();
    if (status != EFI_SUCCESS) return status;
    status = gST->BootServices->LocateProtocol(&gUpdateHttpServiceBindingGuid, (void *)0,
        (void **)&session->binding);
    if (status != EFI_SUCCESS || !session->binding) return status == EFI_SUCCESS ? EFI_NOT_FOUND : status;
    status = session->binding->CreateChild(session->binding, &session->child);
    if (status != EFI_SUCCESS || !session->child) {
        update_http_close(session);
        return status;
    }
    status = gST->BootServices->HandleProtocol(session->child, &gUpdateHttpProtocolGuid,
        (void **)&session->http);
    if (status != EFI_SUCCESS || !session->http) {
        update_http_close(session);
        return status;
    }
    memory_zero(&access, sizeof(access));
    access.UseDefaultAddress = 1;
    memory_zero(&config, sizeof(config));
    config.HttpVersion = HttpVersion11;
    config.TimeOutMillisec = UPDATE_HTTP_TIMEOUT_SECONDS * 1000U;
    config.LocalAddressIsIPv6 = 0;
    config.AccessPoint.IPv4Node = &access;
    for (attempt = 0; attempt < 50U; attempt++) {
        status = session->http->Configure(session->http, &config);
        if (status == EFI_SUCCESS) return EFI_SUCCESS;
        if (status != EFI_NO_MAPPING && status != EFI_NOT_READY) break;
        if (gST->BootServices->Stall) gST->BootServices->Stall(200000U);
    }
    update_http_close(session);
    return update_report_firmware_error("HTTP configuration", status);
}

static int update_http_redirect(EFI_HTTP_STATUS_CODE code) {
    return code == HTTP_STATUS_301_MOVED_PERMANENTLY ||
           code == HTTP_STATUS_302_FOUND ||
           code == HTTP_STATUS_303_SEE_OTHER ||
           code == HTTP_STATUS_307_TEMPORARY_REDIRECT ||
           code == HTTP_STATUS_308_PERMANENT_REDIRECT;
}

static EFI_STATUS update_http_fetch_once(const char *url, UINT8 *buffer, UINTN capacity,
    UINTN *received, EFI_HTTP_STATUS_CODE *responseCode, char *location, UINTN locationCapacity) {
    UPDATE_HTTP_SESSION session;
    EFI_HTTP_REQUEST_DATA requestData;
    EFI_HTTP_RESPONSE_DATA responseData;
    EFI_HTTP_HEADER requestHeaders[4];
    EFI_HTTP_MESSAGE requestMessage;
    EFI_HTTP_MESSAGE responseMessage;
    EFI_HTTP_TOKEN requestToken;
    EFI_HTTP_TOKEN responseToken;
    EFI_EVENT requestEvent = (EFI_EVENT)0;
    EFI_EVENT responseEvent = (EFI_EVENT)0;
    volatile UINT8 requestDone = 0;
    volatile UINT8 responseDone = 0;
    EFI_STATUS status;
    const char *stage = "HTTP setup";
    CHAR16 wideUrl[UPDATE_MAX_URL_BYTES];
    char host[160];
    UINTN expected = 0;
    int hasExpected = 0;
    UINTN used = 0;
    *received = 0;
    *responseCode = HTTP_STATUS_UNSUPPORTED_STATUS;
    if (locationCapacity) location[0] = 0;
    if (!update_https_url(url) || !update_ascii_to_wide(url, wideUrl, UPDATE_MAX_URL_BYTES) ||
        !update_url_host(url, host, sizeof(host))) return EFI_SECURITY_VIOLATION;
    memory_zero(&requestMessage, sizeof(requestMessage));
    memory_zero(&responseMessage, sizeof(responseMessage));
    memory_zero(&requestToken, sizeof(requestToken));
    memory_zero(&responseToken, sizeof(responseToken));
    status = update_http_open(&session);
    if (status != EFI_SUCCESS) return status;
    stage = "request event";
    status = gST->BootServices->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, update_http_notify,
        (void *)&requestDone, &requestEvent);
    if (status != EFI_SUCCESS) goto cleanup;
    stage = "response event";
    status = gST->BootServices->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, update_http_notify,
        (void *)&responseDone, &responseEvent);
    if (status != EFI_SUCCESS) goto cleanup;
    requestData.Method = HttpMethodGet;
    requestData.Url = wideUrl;
    requestHeaders[0].FieldName = "Host";
    requestHeaders[0].FieldValue = host;
    requestHeaders[1].FieldName = "User-Agent";
    requestHeaders[1].FieldValue = "TinyArmOS-Updater/0.5";
    requestHeaders[2].FieldName = "Accept";
    requestHeaders[2].FieldValue = "application/octet-stream";
    requestHeaders[3].FieldName = "Connection";
    requestHeaders[3].FieldValue = "close";
    requestMessage.Data.Request = &requestData;
    requestMessage.HeaderCount = 4;
    requestMessage.Headers = requestHeaders;
    requestToken.Event = requestEvent;
    requestToken.Status = EFI_NOT_READY;
    requestToken.Message = &requestMessage;
    stage = "request queue";
    status = session.http->Request(session.http, &requestToken);
    if (status != EFI_SUCCESS) goto cleanup;
    stage = "request completion";
    status = update_http_wait(&session, &requestToken, &requestDone);
    if (status != EFI_SUCCESS) goto cleanup;
    memory_zero(&responseData, sizeof(responseData));
    memory_zero(&responseMessage, sizeof(responseMessage));
    responseMessage.Data.Response = &responseData;
    responseMessage.BodyLength = capacity;
    responseMessage.Body = buffer;
    responseToken.Event = responseEvent;
    responseToken.Status = EFI_NOT_READY;
    responseToken.Message = &responseMessage;
    stage = "response queue";
    status = session.http->Response(session.http, &responseToken);
    if (status != EFI_SUCCESS) goto cleanup;
    stage = "response completion";
    status = update_http_wait(&session, &responseToken, &responseDone);
    if (status != EFI_SUCCESS && status != EFI_HTTP_ERROR) goto cleanup;
    *responseCode = responseData.StatusCode;
    {
        const char *lengthText = update_find_header(&responseMessage, "Content-Length");
        const char *locationText = update_find_header(&responseMessage, "Location");
        if (lengthText) expected = update_decimal(lengthText, &hasExpected);
        if (locationText && !update_copy(location, locationText, locationCapacity)) {
            status = EFI_OUT_OF_RESOURCES;
            update_free_headers(&responseMessage);
            goto cleanup;
        }
    }
    used = responseMessage.BodyLength;
    update_free_headers(&responseMessage);
    if (update_http_redirect(*responseCode)) {
        status = location[0] ? EFI_SUCCESS : EFI_DEVICE_ERROR;
        goto cleanup;
    }
    if (*responseCode != HTTP_STATUS_200_OK) {
        status = EFI_HTTP_ERROR;
        goto cleanup;
    }
    if (hasExpected && expected > capacity) {
        status = EFI_OUT_OF_RESOURCES;
        goto cleanup;
    }
    while ((!hasExpected || used < expected) && used < capacity) {
        UINTN chunk;
        responseDone = 0;
        memory_zero(&responseMessage, sizeof(responseMessage));
        responseMessage.Data.Response = (EFI_HTTP_RESPONSE_DATA *)0;
        responseMessage.BodyLength = capacity - used;
        responseMessage.Body = buffer + used;
        responseToken.Status = EFI_NOT_READY;
        responseToken.Message = &responseMessage;
        status = session.http->Response(session.http, &responseToken);
        if (status != EFI_SUCCESS) goto cleanup;
        status = update_http_wait(&session, &responseToken, &responseDone);
        if (status != EFI_SUCCESS && status != EFI_HTTP_ERROR) goto cleanup;
        chunk = responseMessage.BodyLength;
        update_free_headers(&responseMessage);
        if (chunk) used += chunk;
        if (status == EFI_HTTP_ERROR || !chunk) {
            status = EFI_SUCCESS;
            break;
        }
    }
    if ((hasExpected && used != expected) || (!hasExpected && used == capacity)) {
        status = EFI_DEVICE_ERROR;
        goto cleanup;
    }
    *received = used;
    status = EFI_SUCCESS;
cleanup:
    if (status != EFI_SUCCESS && status != EFI_HTTP_ERROR)
        update_report_firmware_error(stage, status);
    update_free_headers(&responseMessage);
    if (requestEvent) gST->BootServices->CloseEvent(requestEvent);
    if (responseEvent) gST->BootServices->CloseEvent(responseEvent);
    update_http_close(&session);
    return status;
}

static EFI_STATUS update_http_fetch(const char *initialUrl, UINT8 *buffer, UINTN capacity, UINTN *received) {
    char current[UPDATE_MAX_URL_BYTES];
    char location[UPDATE_MAX_URL_BYTES];
    char next[UPDATE_MAX_URL_BYTES];
    UINTN redirect;
    if (!update_copy(current, initialUrl, sizeof(current))) return EFI_OUT_OF_RESOURCES;
    for (redirect = 0; redirect <= UPDATE_MAX_REDIRECTS; redirect++) {
        EFI_HTTP_STATUS_CODE responseCode;
        EFI_STATUS status = update_http_fetch_once(current, buffer, capacity, received,
            &responseCode, location, sizeof(location));
        if (status != EFI_SUCCESS) return status;
        if (responseCode == HTTP_STATUS_200_OK) return EFI_SUCCESS;
        if (!update_http_redirect(responseCode) || redirect == UPDATE_MAX_REDIRECTS ||
            !update_resolve_redirect(current, location, next, sizeof(next)) || !update_https_url(next)) {
            return EFI_SECURITY_VIOLATION;
        }
        if (!update_copy(current, next, sizeof(current))) return EFI_OUT_OF_RESOURCES;
    }
    return EFI_ABORTED;
}

static int update_parse_manifest(char *text, UINTN bytes, UPDATE_MANIFEST *manifest) {
    char *cursor = text;
    int haveVersion = 0;
    int haveSize = 0;
    int haveDigest = 0;
    int haveUrl = 0;
    if (bytes >= UPDATE_MAX_MANIFEST_BYTES) return 0;
    text[bytes] = 0;
    memory_zero(manifest, sizeof(*manifest));
    while (*cursor) {
        char *line = cursor;
        char *equals;
        while (*cursor && *cursor != '\n' && *cursor != '\r') cursor++;
        if (*cursor) {
            *cursor++ = 0;
            while (*cursor == '\n' || *cursor == '\r') cursor++;
        }
        if (!*line || *line == '#') continue;
        equals = line;
        while (*equals && *equals != '=') equals++;
        if (*equals != '=') return 0;
        *equals++ = 0;
        if (streq(line, "version")) {
            if (haveVersion || !update_copy(manifest->version, equals, sizeof(manifest->version))) return 0;
            haveVersion = 1;
        } else if (streq(line, "size")) {
            int okay;
            if (haveSize) return 0;
            manifest->size = update_decimal(equals, &okay);
            if (!okay || !manifest->size || manifest->size > UPDATE_MAX_IMAGE_BYTES) return 0;
            haveSize = 1;
        } else if (streq(line, "sha256")) {
            if (haveDigest || !update_parse_digest(equals, manifest->digest)) return 0;
            haveDigest = 1;
        } else if (streq(line, "url")) {
            if (haveUrl || !update_https_url(equals) ||
                !starts_with(equals, "https://github.com/firesafetylite/TinyArmOS/releases/download/") ||
                !update_copy(manifest->url, equals, sizeof(manifest->url))) return 0;
            haveUrl = 1;
        } else return 0;
    }
    return haveVersion && haveSize && haveDigest && haveUrl;
}

static UINT16 update_le16(const UINT8 *data) {
    return (UINT16)((UINT16)data[0] | ((UINT16)data[1] << 8));
}

static UINT32 update_le32(const UINT8 *data) {
    return (UINT32)data[0] | ((UINT32)data[1] << 8) |
           ((UINT32)data[2] << 16) | ((UINT32)data[3] << 24);
}

static int update_valid_efi(const UINT8 *image, UINTN bytes) {
    UINT32 peOffset;
    UINTN optionalOffset;
    if (bytes < 512U || image[0] != 'M' || image[1] != 'Z') return 0;
    peOffset = update_le32(image + 0x3cU);
    if ((UINTN)peOffset + 96U > bytes) return 0;
    if (image[peOffset] != 'P' || image[peOffset + 1U] != 'E' ||
        image[peOffset + 2U] || image[peOffset + 3U]) return 0;
    if (update_le16(image + peOffset + 4U) != 0xaa64U) return 0;
    optionalOffset = (UINTN)peOffset + 24U;
    if (update_le16(image + optionalOffset) != 0x20bU) return 0;
    return update_le16(image + optionalOffset + 68U) == 10U;
}

static EFI_STATUS update_delete_file(const CHAR16 *path) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    EFI_STATUS status = gVolumeRoot->Open(gVolumeRoot, &file, (CHAR16 *)path,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (status == EFI_NOT_FOUND) return EFI_SUCCESS;
    if (status != EFI_SUCCESS || !file) return status;
    return file->Delete(file);
}

static EFI_STATUS update_write_file(const CHAR16 *path, const UINT8 *data, UINTN bytes) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    EFI_STATUS status;
    UINTN offset = 0;
    status = update_delete_file(path);
    if (status != EFI_SUCCESS) return status;
    status = gVolumeRoot->Open(gVolumeRoot, &file, (CHAR16 *)path,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (status != EFI_SUCCESS || !file) return status;
    while (offset < bytes) {
        UINTN chunk = bytes - offset;
        if (chunk > 65536U) chunk = 65536U;
        status = file->Write(file, &chunk, (void *)(data + offset));
        if (status != EFI_SUCCESS || !chunk) break;
        offset += chunk;
    }
    if (status == EFI_SUCCESS && offset == bytes) status = file->Flush(file);
    file->Close(file);
    if (status != EFI_SUCCESS || offset != bytes) {
        update_delete_file(path);
        return status == EFI_SUCCESS ? EFI_DEVICE_ERROR : status;
    }
    return EFI_SUCCESS;
}

static EFI_STATUS update_read_file(const CHAR16 *path, UINT8 *data, UINTN capacity, UINTN *bytesRead) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    EFI_STATUS status;
    UINTN offset = 0;
    status = gVolumeRoot->Open(gVolumeRoot, &file, (CHAR16 *)path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || !file) return status;
    while (offset < capacity) {
        UINTN chunk = capacity - offset;
        if (chunk > 65536U) chunk = 65536U;
        status = file->Read(file, &chunk, data + offset);
        if (status != EFI_SUCCESS || !chunk) break;
        offset += chunk;
    }
    file->Close(file);
    if (status != EFI_SUCCESS) return status;
    *bytesRead = offset;
    return EFI_SUCCESS;
}

static EFI_STATUS update_hash_file(const CHAR16 *path, UINTN *fileSize, UINT8 digest[32]) {
    EFI_FILE_PROTOCOL *file = (EFI_FILE_PROTOCOL *)0;
    UPDATE_SHA256 sha;
    UINT8 buffer[4096];
    EFI_STATUS status;
    UINTN total = 0;
    status = gVolumeRoot->Open(gVolumeRoot, &file, (CHAR16 *)path, EFI_FILE_MODE_READ, 0);
    if (status != EFI_SUCCESS || !file) return status;
    update_sha256_init(&sha);
    for (;;) {
        UINTN chunk = sizeof(buffer);
        status = file->Read(file, &chunk, buffer);
        if (status != EFI_SUCCESS || !chunk) break;
        if (total + chunk > UPDATE_MAX_IMAGE_BYTES) {
            status = EFI_OUT_OF_RESOURCES;
            break;
        }
        update_sha256_add(&sha, buffer, chunk);
        total += chunk;
    }
    file->Close(file);
    if (status != EFI_SUCCESS) return status;
    update_sha256_finish(&sha, digest);
    *fileSize = total;
    return EFI_SUCCESS;
}

static EFI_STATUS update_install_image(const UINT8 *image, UINTN imageSize, const UINT8 digest[32]) {
    UINT8 *oldImage = (UINT8 *)0;
    UINTN oldSize = 0;
    UINTN verifiedSize = 0;
    UINT8 verifiedDigest[32];
    EFI_STATUS status;
    if (!gVolumeRoot) return EFI_NOT_FOUND;
    status = gST->BootServices->AllocatePool(2, UPDATE_MAX_IMAGE_BYTES, (void **)&oldImage);
    if (status != EFI_SUCCESS || !oldImage) return EFI_OUT_OF_RESOURCES;
    status = update_read_file(gUpdateBootPath, oldImage, UPDATE_MAX_IMAGE_BYTES, &oldSize);
    if (status != EFI_SUCCESS || !oldSize) goto cleanup;
    status = update_write_file(gUpdateBackupPath, oldImage, oldSize);
    if (status != EFI_SUCCESS) goto cleanup;
    status = update_write_file(gUpdateStagePath, image, imageSize);
    if (status != EFI_SUCCESS) goto cleanup;
    status = update_hash_file(gUpdateStagePath, &verifiedSize, verifiedDigest);
    if (status != EFI_SUCCESS || verifiedSize != imageSize || !update_digest_equal(verifiedDigest, digest)) {
        status = EFI_SECURITY_VIOLATION;
        goto cleanup;
    }
    status = update_write_file(gUpdateBootPath, image, imageSize);
    if (status != EFI_SUCCESS) {
        update_write_file(gUpdateBootPath, oldImage, oldSize);
        goto cleanup;
    }
    status = update_hash_file(gUpdateBootPath, &verifiedSize, verifiedDigest);
    if (status != EFI_SUCCESS || verifiedSize != imageSize || !update_digest_equal(verifiedDigest, digest)) {
        update_write_file(gUpdateBootPath, oldImage, oldSize);
        status = EFI_SECURITY_VIOLATION;
        goto cleanup;
    }
    update_delete_file(gUpdateStagePath);
    status = EFI_SUCCESS;
cleanup:
    if (status != EFI_SUCCESS) update_delete_file(gUpdateStagePath);
    gST->BootServices->FreePool(oldImage);
    return status;
}

static void update_print_status(const char *operation, EFI_STATUS status) {
    print(operation);
    print(" failed (UEFI status ");
    print_hex(status);
    print(").\n");
    if (status == EFI_NOT_FOUND || status == EFI_UNSUPPORTED) {
        print("This firmware does not expose the UEFI HTTP/TLS service.\n");
    } else if (status == EFI_NO_MAPPING || status == EFI_NOT_READY) {
        print("The network has not received an IPv4/DHCP configuration.\n");
    } else if (status == EFI_SECURITY_VIOLATION) {
        print("HTTPS, redirect, manifest, or checksum verification was rejected.\n");
    } else if (status == EFI_TIMEOUT) {
        print("The firmware network request timed out.\n");
    }
}

static void command_update(int checkOnly) {
    UINT8 *manifestBytes = (UINT8 *)0;
    UINT8 *image = (UINT8 *)0;
    UINTN manifestSize = 0;
    UINTN imageSize = 0;
    UINT8 actualDigest[32];
    UPDATE_MANIFEST manifest;
    EFI_STATUS status;
    char answer[32];
    print("Checking GitHub Releases over firmware HTTPS...\n");
    status = gST->BootServices->AllocatePool(2, UPDATE_MAX_MANIFEST_BYTES + 1U, (void **)&manifestBytes);
    if (status != EFI_SUCCESS || !manifestBytes) {
        update_print_status("Manifest allocation", EFI_OUT_OF_RESOURCES);
        return;
    }
    status = update_http_fetch(UPDATE_MANIFEST_URL, manifestBytes, UPDATE_MAX_MANIFEST_BYTES, &manifestSize);
    if (status != EFI_SUCCESS) {
        update_print_status("Update check", status);
        goto cleanup;
    }
    if (!update_parse_manifest((char *)manifestBytes, manifestSize, &manifest)) {
        update_print_status("Manifest validation", EFI_SECURITY_VIOLATION);
        goto cleanup;
    }
    print("Latest release: ");
    print(manifest.version);
    print(" (current ");
    print(TINYARMOS_VERSION);
    print(")\n");
    if (streq(manifest.version, TINYARMOS_VERSION)) {
        print("TinyArmOS is up to date.\n");
        goto cleanup;
    }
    if (checkOnly) {
        print("Update available. Run 'update' to install it.\n");
        goto cleanup;
    }
    print("Download and install this update? Type UPDATE: ");
    read_line(answer, sizeof(answer));
    if (!streq(answer, "UPDATE")) {
        print("update cancelled\n");
        goto cleanup;
    }
    status = gST->BootServices->AllocatePool(2, manifest.size, (void **)&image);
    if (status != EFI_SUCCESS || !image) {
        update_print_status("Image allocation", EFI_OUT_OF_RESOURCES);
        goto cleanup;
    }
    print("Downloading ");
    print_u64(manifest.size);
    print(" bytes...\n");
    status = update_http_fetch(manifest.url, image, manifest.size, &imageSize);
    if (status != EFI_SUCCESS) {
        update_print_status("Update download", status);
        goto cleanup;
    }
    update_sha256_bytes(image, imageSize, actualDigest);
    if (imageSize != manifest.size || !update_digest_equal(actualDigest, manifest.digest) ||
        !update_valid_efi(image, imageSize)) {
        update_print_status("Image verification", EFI_SECURITY_VIOLATION);
        goto cleanup;
    }
    print("Verified ARM64 EFI image. Installing with BOOTAA64.BAK fallback...\n");
    status = update_install_image(image, imageSize, manifest.digest);
    if (status != EFI_SUCCESS) {
        update_print_status("Update installation", status);
        goto cleanup;
    }
    print("TinyArmOS ");
    print(manifest.version);
    print(" installed. Type REBOOT to restart now: ");
    read_line(answer, sizeof(answer));
    if (streq(answer, "REBOOT")) {
        gST->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, (void *)0);
        for (;;) __asm__ volatile("wfe");
    }
    print("Run 'reboot' before continuing normal use.\n");
cleanup:
    if (image) gST->BootServices->FreePool(image);
    if (manifestBytes) gST->BootServices->FreePool(manifestBytes);
}
