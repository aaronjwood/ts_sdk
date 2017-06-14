/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef VERIZON_MQTT_CA_H
#define VERIZON_MQTT_CA_H

#if 0
static const unsigned char cacert_pem[] = {
	"-----BEGIN CERTIFICATE-----\n"
	"MIIFqTCCBJGgAwIBAgIUNSxGSj/c70oyGmmWXem44mKSN5AwDQYJKoZIhvcNAQEL\n"
	"BQAwgY0xCzAJBgNVBAYTAk5MMRIwEAYDVQQHEwlBbXN0ZXJkYW0xJTAjBgNVBAoT\n"
	"HFZlcml6b24gRW50ZXJwcmlzZSBTb2x1dGlvbnMxEzARBgNVBAsTCkN5YmVydHJ1\n"
	"c3QxLjAsBgNVBAMTJVZlcml6b24gUHVibGljIFN1cmVTZXJ2ZXIgQ0EgRzE0LVNI\n"
	"QTIwHhcNMTcwNDEwMjM0ODE1WhcNMTkwNDEwMjM0ODE1WjCBnzELMAkGA1UEBhMC\n"
	"VVMxFjAUBgNVBAgTDU1hc3NhY2h1c2V0dHMxEDAOBgNVBAcTB1dhbHRoYW0xIjAg\n"
	"BgNVBAoTGVZlcml6b24gRGF0YSBTZXJ2aWNlcyBMTEMxFDASBgNVBAsTC1Zlcml6\n"
	"b24gTlBEMSwwKgYDVQQDEyNzaW1wbS1lYS1pd2sudGhpbmdzcGFjZS52ZXJpem9u\n"
	"LmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAM7PDo+ozn2pd8CM\n"
	"3vUdkvKyCyJFB8NG0RKJlc6gLZ8aP1u36F8sUDzvo+0QqSL/3wariNXnWeqNIEz8\n"
	"+tpGCqWAq7s4oFCn+F+TcHaRAx3N9ot3t5mukQYNQzD7KYHpTYGDf0CJ1wZqjBSA\n"
	"WzhOdbykdvksL75bAmGL/Zq8/jiLXQMaOSpJTvvOIoxon0Q1gFEFet034KHERCCL\n"
	"Umcm0IA78T/F/GLVurCkrxSxeTb2A9d58T/n8uypIVlniWHY2rpOrkYjLcqwIsgK\n"
	"JLUEByT9YPBTk4fQ9HUBA9VTjvb8sAbLfp4wd5X119UAM8r5wgoZdIUciMLU4kNh\n"
	"NvzEGEMCAwEAAaOCAeswggHnMAwGA1UdEwEB/wQCMAAwTAYDVR0gBEUwQzBBBgkr\n"
	"BgEEAbE+ATIwNDAyBggrBgEFBQcCARYmaHR0cHM6Ly9zZWN1cmUub21uaXJvb3Qu\n"
	"Y29tL3JlcG9zaXRvcnkwgakGCCsGAQUFBwEBBIGcMIGZMC0GCCsGAQUFBzABhiFo\n"
	"dHRwOi8vdnBzc2cxNDIub2NzcC5vbW5pcm9vdC5jb20wMwYIKwYBBQUHMAKGJ2h0\n"
	"dHA6Ly9jYWNlcnQub21uaXJvb3QuY29tL3Zwc3NnMTQyLmNydDAzBggrBgEFBQcw\n"
	"AoYnaHR0cDovL2NhY2VydC5vbW5pcm9vdC5jb20vdnBzc2cxNDIuZGVyMC4GA1Ud\n"
	"EQQnMCWCI3NpbXBtLWVhLWl3ay50aGluZ3NwYWNlLnZlcml6b24uY29tMA4GA1Ud\n"
	"DwEB/wQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwHwYDVR0j\n"
	"BBgwFoAU5C27kQFlJh+0ej+jFSWkzoxEMzswPgYDVR0fBDcwNTAzoDGgL4YtaHR0\n"
	"cDovL3Zwc3NnMTQyLmNybC5vbW5pcm9vdC5jb20vdnBzc2cxNDIuY3JsMB0GA1Ud\n"
	"DgQWBBTAJU3Q/0JgNgVi7ZeWOSGhd/Y2vzANBgkqhkiG9w0BAQsFAAOCAQEAxkG4\n"
	"H+RK93IfJ3Las1yLjDgXB+l+hmR+B+kwQ3kc+i0ZKnxOM/yZymc/ck7Z9/HiJb7A\n"
	"Xa0xH+17NbF1RJZd7t/1HhdvNdBYR1Jo2GcTfmapylM4i/YeaC0PmcqwS5xnHBY1\n"
	"zidnNaJU9K1C+M9MUhi3Kz3088+g+GJpoaIEUmsd0qS/pRAE9jhB2IYPX1W3nUFK\n"
	"4FLOEZRynInfWyfJ3RY9ofxekvB5P0wC/GwztVzqb++8wjh/oWzUXSNKfMuQg7WT\n"
	"R9m/C1LMQNafrgobfI5Kogp+jsZLbII0f2FbfXj+NyBrTZvJI3xytsGzkRR2IMoU\n"
	"twBQEqvhFznO2hAfQw==\n"
	"-----END CERTIFICATE-----\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIIFqTCCBJGgAwIBAgIUNSxGSj/c70oyGmmWXem44mKSN5AwDQYJKoZIhvcNAQEL\n"
	"BQAwgY0xCzAJBgNVBAYTAk5MMRIwEAYDVQQHEwlBbXN0ZXJkYW0xJTAjBgNVBAoT\n"
	"HFZlcml6b24gRW50ZXJwcmlzZSBTb2x1dGlvbnMxEzARBgNVBAsTCkN5YmVydHJ1\n"
	"c3QxLjAsBgNVBAMTJVZlcml6b24gUHVibGljIFN1cmVTZXJ2ZXIgQ0EgRzE0LVNI\n"
	"QTIwHhcNMTcwNDEwMjM0ODE1WhcNMTkwNDEwMjM0ODE1WjCBnzELMAkGA1UEBhMC\n"
	"VVMxFjAUBgNVBAgTDU1hc3NhY2h1c2V0dHMxEDAOBgNVBAcTB1dhbHRoYW0xIjAg\n"
	"BgNVBAoTGVZlcml6b24gRGF0YSBTZXJ2aWNlcyBMTEMxFDASBgNVBAsTC1Zlcml6\n"
	"b24gTlBEMSwwKgYDVQQDEyNzaW1wbS1lYS1pd2sudGhpbmdzcGFjZS52ZXJpem9u\n"
	"LmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAM7PDo+ozn2pd8CM\n"
	"3vUdkvKyCyJFB8NG0RKJlc6gLZ8aP1u36F8sUDzvo+0QqSL/3wariNXnWeqNIEz8\n"
	"+tpGCqWAq7s4oFCn+F+TcHaRAx3N9ot3t5mukQYNQzD7KYHpTYGDf0CJ1wZqjBSA\n"
	"WzhOdbykdvksL75bAmGL/Zq8/jiLXQMaOSpJTvvOIoxon0Q1gFEFet034KHERCCL\n"
	"Umcm0IA78T/F/GLVurCkrxSxeTb2A9d58T/n8uypIVlniWHY2rpOrkYjLcqwIsgK\n"
	"JLUEByT9YPBTk4fQ9HUBA9VTjvb8sAbLfp4wd5X119UAM8r5wgoZdIUciMLU4kNh\n"
	"NvzEGEMCAwEAAaOCAeswggHnMAwGA1UdEwEB/wQCMAAwTAYDVR0gBEUwQzBBBgkr\n"
	"BgEEAbE+ATIwNDAyBggrBgEFBQcCARYmaHR0cHM6Ly9zZWN1cmUub21uaXJvb3Qu\n"
	"Y29tL3JlcG9zaXRvcnkwgakGCCsGAQUFBwEBBIGcMIGZMC0GCCsGAQUFBzABhiFo\n"
	"dHRwOi8vdnBzc2cxNDIub2NzcC5vbW5pcm9vdC5jb20wMwYIKwYBBQUHMAKGJ2h0\n"
	"dHA6Ly9jYWNlcnQub21uaXJvb3QuY29tL3Zwc3NnMTQyLmNydDAzBggrBgEFBQcw\n"
	"AoYnaHR0cDovL2NhY2VydC5vbW5pcm9vdC5jb20vdnBzc2cxNDIuZGVyMC4GA1Ud\n"
	"EQQnMCWCI3NpbXBtLWVhLWl3ay50aGluZ3NwYWNlLnZlcml6b24uY29tMA4GA1Ud\n"
	"DwEB/wQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwHwYDVR0j\n"
	"BBgwFoAU5C27kQFlJh+0ej+jFSWkzoxEMzswPgYDVR0fBDcwNTAzoDGgL4YtaHR0\n"
	"cDovL3Zwc3NnMTQyLmNybC5vbW5pcm9vdC5jb20vdnBzc2cxNDIuY3JsMB0GA1Ud\n"
	"DgQWBBTAJU3Q/0JgNgVi7ZeWOSGhd/Y2vzANBgkqhkiG9w0BAQsFAAOCAQEAxkG4\n"
	"H+RK93IfJ3Las1yLjDgXB+l+hmR+B+kwQ3kc+i0ZKnxOM/yZymc/ck7Z9/HiJb7A\n"
	"Xa0xH+17NbF1RJZd7t/1HhdvNdBYR1Jo2GcTfmapylM4i/YeaC0PmcqwS5xnHBY1\n"
	"zidnNaJU9K1C+M9MUhi3Kz3088+g+GJpoaIEUmsd0qS/pRAE9jhB2IYPX1W3nUFK\n"
	"4FLOEZRynInfWyfJ3RY9ofxekvB5P0wC/GwztVzqb++8wjh/oWzUXSNKfMuQg7WT\n"
	"R9m/C1LMQNafrgobfI5Kogp+jsZLbII0f2FbfXj+NyBrTZvJI3xytsGzkRR2IMoU\n"
	"twBQEqvhFznO2hAfQw==\n"
	"-----END CERTIFICATE-----\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIIFHzCCBAegAwIBAgIEByemYTANBgkqhkiG9w0BAQsFADBaMQswCQYDVQQGEwJJ\n"
	"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n"
	"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTE0MDQwOTE2MDMwNloX\n"
	"DTIxMDQwOTE2MDIxMFowgY0xCzAJBgNVBAYTAk5MMRIwEAYDVQQHEwlBbXN0ZXJk\n"
	"YW0xJTAjBgNVBAoTHFZlcml6b24gRW50ZXJwcmlzZSBTb2x1dGlvbnMxEzARBgNV\n"
	"BAsTCkN5YmVydHJ1c3QxLjAsBgNVBAMTJVZlcml6b24gUHVibGljIFN1cmVTZXJ2\n"
	"ZXIgQ0EgRzE0LVNIQTIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDo\n"
	"OwWImO3Kdhcb8lmJVOROdTidMxYJasOStOvLgnHWmzyn6VbeP1xeZbS8mIyHxY0m\n"
	"916FlYdsWd3sKV+wnXFBZCnYJEvLTABe1dT3Tob4acjGEonr9RKiDbeBb6LcRA3Y\n"
	"YHMmdnRAfzHtOAPj7CqVcSFdcib7HuTTf2Ot+xEzJkRyh7dPs+OcXvo0mBp8Oyga\n"
	"TBTZiMg3d5t2UVZQbNf+3O2BMBVhnixXTA7JDEYf0RfLPdbbKjYxE9yez4ICwURL\n"
	"JwSQUv/9Ylrb65NAVWpfmmceOU7LH0XvHy+6KHIZtnIxP53/w/X3Ui+X4o2R30jt\n"
	"iDdq3McycOlB/Bv2zqAhAgMBAAGjggG3MIIBszASBgNVHRMBAf8ECDAGAQH/AgEA\n"
	"MEwGA1UdIARFMEMwQQYJKwYBBAGxPgEyMDQwMgYIKwYBBQUHAgEWJmh0dHBzOi8v\n"
	"c2VjdXJlLm9tbmlyb290LmNvbS9yZXBvc2l0b3J5MIG6BggrBgEFBQcBAQSBrTCB\n"
	"qjAyBggrBgEFBQcwAYYmaHR0cDovL29jc3Aub21uaXJvb3QuY29tL2JhbHRpbW9y\n"
	"ZXJvb3QwOQYIKwYBBQUHMAKGLWh0dHBzOi8vY2FjZXJ0Lm9tbmlyb290LmNvbS9i\n"
	"YWx0aW1vcmVyb290LmNydDA5BggrBgEFBQcwAoYtaHR0cHM6Ly9jYWNlcnQub21u\n"
	"aXJvb3QuY29tL2JhbHRpbW9yZXJvb3QuZGVyMA4GA1UdDwEB/wQEAwIBxjAfBgNV\n"
	"HSMEGDAWgBTlnVkwgkdYzKz6CFQ2hns6tQRN8DBCBgNVHR8EOzA5MDegNaAzhjFo\n"
	"dHRwOi8vY2RwMS5wdWJsaWMtdHJ1c3QuY29tL0NSTC9PbW5pcm9vdDIwMjUuY3Js\n"
	"MB0GA1UdDgQWBBTkLbuRAWUmH7R6P6MVJaTOjEQzOzANBgkqhkiG9w0BAQsFAAOC\n"
	"AQEAdA8XA5Qvi4LTuB/IeCu44Bv/dFrvuhtUYUmrm1ZxQcbJ0Z8Vqyl54AT0tN8d\n"
	"PmvBQk+ORWF2Ncoa7dvVX3UBT0Hnu2lITWEpq7ul6qFp6QWwnev2T8JlKJ6oeCCW\n"
	"qmu71/Eo7YfYsxmhdwyPMZpwjYhyGXBdBhooEuoJLg9S2hUiyPdxG52Pr1pZJti5\n"
	"A9dEYJfZ5etyfQyd+aO51rA8jpe3rpMS8blSBkfid8GrSawwSDvUTGFC18om4K70\n"
	"vPHTStzKNAciUKSdAEY6VNzxkJOu6IkR5H3f6Sw/Mq9EOK9jg6t0qGWwu24WunkO\n"
	"9KlgK++DdBLm13arqkkvSebisQ==\n"
	"-----END CERTIFICATE-----\n"
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n"
	"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n"
	"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n"
	"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n"
	"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n"
	"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n"
	"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n"
	"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n"
	"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n"
	"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n"
	"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n"
	"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n"
	"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n"
	"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n"
	"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n"
	"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n"
	"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n"
	"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n"
	"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n"
	"-----END CERTIFICATE-----\n"
};
#endif

#if 1
static const unsigned char cacert_buf[] = {
	0x30, 0x82, 0x05, 0xa9, 0x30, 0x82, 0x04, 0x91, 0xa0, 0x03, 0x02, 0x01,
	0x02, 0x02, 0x14, 0x35, 0x2c, 0x46, 0x4a, 0x3f, 0xdc, 0xef, 0x4a, 0x32,
	0x1a, 0x69, 0x96, 0x5d, 0xe9, 0xb8, 0xe2, 0x62, 0x92, 0x37, 0x90, 0x30,
	0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b,
	0x05, 0x00, 0x30, 0x81, 0x8d, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55,
	0x04, 0x06, 0x13, 0x02, 0x4e, 0x4c, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03,
	0x55, 0x04, 0x07, 0x13, 0x09, 0x41, 0x6d, 0x73, 0x74, 0x65, 0x72, 0x64,
	0x61, 0x6d, 0x31, 0x25, 0x30, 0x23, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13,
	0x1c, 0x56, 0x65, 0x72, 0x69, 0x7a, 0x6f, 0x6e, 0x20, 0x45, 0x6e, 0x74,
	0x65, 0x72, 0x70, 0x72, 0x69, 0x73, 0x65, 0x20, 0x53, 0x6f, 0x6c, 0x75,
	0x74, 0x69, 0x6f, 0x6e, 0x73, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55,
	0x04, 0x0b, 0x13, 0x0a, 0x43, 0x79, 0x62, 0x65, 0x72, 0x74, 0x72, 0x75,
	0x73, 0x74, 0x31, 0x2e, 0x30, 0x2c, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
	0x25, 0x56, 0x65, 0x72, 0x69, 0x7a, 0x6f, 0x6e, 0x20, 0x50, 0x75, 0x62,
	0x6c, 0x69, 0x63, 0x20, 0x53, 0x75, 0x72, 0x65, 0x53, 0x65, 0x72, 0x76,
	0x65, 0x72, 0x20, 0x43, 0x41, 0x20, 0x47, 0x31, 0x34, 0x2d, 0x53, 0x48,
	0x41, 0x32, 0x30, 0x1e, 0x17, 0x0d, 0x31, 0x37, 0x30, 0x34, 0x31, 0x30,
	0x32, 0x33, 0x34, 0x38, 0x31, 0x35, 0x5a, 0x17, 0x0d, 0x31, 0x39, 0x30,
	0x34, 0x31, 0x30, 0x32, 0x33, 0x34, 0x38, 0x31, 0x35, 0x5a, 0x30, 0x81,
	0x9f, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
	0x55, 0x53, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13,
	0x0d, 0x4d, 0x61, 0x73, 0x73, 0x61, 0x63, 0x68, 0x75, 0x73, 0x65, 0x74,
	0x74, 0x73, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13,
	0x07, 0x57, 0x61, 0x6c, 0x74, 0x68, 0x61, 0x6d, 0x31, 0x22, 0x30, 0x20,
	0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x19, 0x56, 0x65, 0x72, 0x69, 0x7a,
	0x6f, 0x6e, 0x20, 0x44, 0x61, 0x74, 0x61, 0x20, 0x53, 0x65, 0x72, 0x76,
	0x69, 0x63, 0x65, 0x73, 0x20, 0x4c, 0x4c, 0x43, 0x31, 0x14, 0x30, 0x12,
	0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x0b, 0x56, 0x65, 0x72, 0x69, 0x7a,
	0x6f, 0x6e, 0x20, 0x4e, 0x50, 0x44, 0x31, 0x2c, 0x30, 0x2a, 0x06, 0x03,
	0x55, 0x04, 0x03, 0x13, 0x23, 0x73, 0x69, 0x6d, 0x70, 0x6d, 0x2d, 0x65,
	0x61, 0x2d, 0x69, 0x77, 0x6b, 0x2e, 0x74, 0x68, 0x69, 0x6e, 0x67, 0x73,
	0x70, 0x61, 0x63, 0x65, 0x2e, 0x76, 0x65, 0x72, 0x69, 0x7a, 0x6f, 0x6e,
	0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09,
	0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03,
	0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
	0x00, 0xce, 0xcf, 0x0e, 0x8f, 0xa8, 0xce, 0x7d, 0xa9, 0x77, 0xc0, 0x8c,
	0xde, 0xf5, 0x1d, 0x92, 0xf2, 0xb2, 0x0b, 0x22, 0x45, 0x07, 0xc3, 0x46,
	0xd1, 0x12, 0x89, 0x95, 0xce, 0xa0, 0x2d, 0x9f, 0x1a, 0x3f, 0x5b, 0xb7,
	0xe8, 0x5f, 0x2c, 0x50, 0x3c, 0xef, 0xa3, 0xed, 0x10, 0xa9, 0x22, 0xff,
	0xdf, 0x06, 0xab, 0x88, 0xd5, 0xe7, 0x59, 0xea, 0x8d, 0x20, 0x4c, 0xfc,
	0xfa, 0xda, 0x46, 0x0a, 0xa5, 0x80, 0xab, 0xbb, 0x38, 0xa0, 0x50, 0xa7,
	0xf8, 0x5f, 0x93, 0x70, 0x76, 0x91, 0x03, 0x1d, 0xcd, 0xf6, 0x8b, 0x77,
	0xb7, 0x99, 0xae, 0x91, 0x06, 0x0d, 0x43, 0x30, 0xfb, 0x29, 0x81, 0xe9,
	0x4d, 0x81, 0x83, 0x7f, 0x40, 0x89, 0xd7, 0x06, 0x6a, 0x8c, 0x14, 0x80,
	0x5b, 0x38, 0x4e, 0x75, 0xbc, 0xa4, 0x76, 0xf9, 0x2c, 0x2f, 0xbe, 0x5b,
	0x02, 0x61, 0x8b, 0xfd, 0x9a, 0xbc, 0xfe, 0x38, 0x8b, 0x5d, 0x03, 0x1a,
	0x39, 0x2a, 0x49, 0x4e, 0xfb, 0xce, 0x22, 0x8c, 0x68, 0x9f, 0x44, 0x35,
	0x80, 0x51, 0x05, 0x7a, 0xdd, 0x37, 0xe0, 0xa1, 0xc4, 0x44, 0x20, 0x8b,
	0x52, 0x67, 0x26, 0xd0, 0x80, 0x3b, 0xf1, 0x3f, 0xc5, 0xfc, 0x62, 0xd5,
	0xba, 0xb0, 0xa4, 0xaf, 0x14, 0xb1, 0x79, 0x36, 0xf6, 0x03, 0xd7, 0x79,
	0xf1, 0x3f, 0xe7, 0xf2, 0xec, 0xa9, 0x21, 0x59, 0x67, 0x89, 0x61, 0xd8,
	0xda, 0xba, 0x4e, 0xae, 0x46, 0x23, 0x2d, 0xca, 0xb0, 0x22, 0xc8, 0x0a,
	0x24, 0xb5, 0x04, 0x07, 0x24, 0xfd, 0x60, 0xf0, 0x53, 0x93, 0x87, 0xd0,
	0xf4, 0x75, 0x01, 0x03, 0xd5, 0x53, 0x8e, 0xf6, 0xfc, 0xb0, 0x06, 0xcb,
	0x7e, 0x9e, 0x30, 0x77, 0x95, 0xf5, 0xd7, 0xd5, 0x00, 0x33, 0xca, 0xf9,
	0xc2, 0x0a, 0x19, 0x74, 0x85, 0x1c, 0x88, 0xc2, 0xd4, 0xe2, 0x43, 0x61,
	0x36, 0xfc, 0xc4, 0x18, 0x43, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x82,
	0x01, 0xeb, 0x30, 0x82, 0x01, 0xe7, 0x30, 0x0c, 0x06, 0x03, 0x55, 0x1d,
	0x13, 0x01, 0x01, 0xff, 0x04, 0x02, 0x30, 0x00, 0x30, 0x4c, 0x06, 0x03,
	0x55, 0x1d, 0x20, 0x04, 0x45, 0x30, 0x43, 0x30, 0x41, 0x06, 0x09, 0x2b,
	0x06, 0x01, 0x04, 0x01, 0xb1, 0x3e, 0x01, 0x32, 0x30, 0x34, 0x30, 0x32,
	0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x02, 0x01, 0x16, 0x26,
	0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x73, 0x65, 0x63, 0x75,
	0x72, 0x65, 0x2e, 0x6f, 0x6d, 0x6e, 0x69, 0x72, 0x6f, 0x6f, 0x74, 0x2e,
	0x63, 0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f,
	0x72, 0x79, 0x30, 0x81, 0xa9, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05,
	0x07, 0x01, 0x01, 0x04, 0x81, 0x9c, 0x30, 0x81, 0x99, 0x30, 0x2d, 0x06,
	0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x86, 0x21, 0x68,
	0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x76, 0x70, 0x73, 0x73, 0x67, 0x31,
	0x34, 0x32, 0x2e, 0x6f, 0x63, 0x73, 0x70, 0x2e, 0x6f, 0x6d, 0x6e, 0x69,
	0x72, 0x6f, 0x6f, 0x74, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x33, 0x06, 0x08,
	0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x02, 0x86, 0x27, 0x68, 0x74,
	0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x61, 0x63, 0x65, 0x72, 0x74, 0x2e,
	0x6f, 0x6d, 0x6e, 0x69, 0x72, 0x6f, 0x6f, 0x74, 0x2e, 0x63, 0x6f, 0x6d,
	0x2f, 0x76, 0x70, 0x73, 0x73, 0x67, 0x31, 0x34, 0x32, 0x2e, 0x63, 0x72,
	0x74, 0x30, 0x33, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30,
	0x02, 0x86, 0x27, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x61,
	0x63, 0x65, 0x72, 0x74, 0x2e, 0x6f, 0x6d, 0x6e, 0x69, 0x72, 0x6f, 0x6f,
	0x74, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x76, 0x70, 0x73, 0x73, 0x67, 0x31,
	0x34, 0x32, 0x2e, 0x64, 0x65, 0x72, 0x30, 0x2e, 0x06, 0x03, 0x55, 0x1d,
	0x11, 0x04, 0x27, 0x30, 0x25, 0x82, 0x23, 0x73, 0x69, 0x6d, 0x70, 0x6d,
	0x2d, 0x65, 0x61, 0x2d, 0x69, 0x77, 0x6b, 0x2e, 0x74, 0x68, 0x69, 0x6e,
	0x67, 0x73, 0x70, 0x61, 0x63, 0x65, 0x2e, 0x76, 0x65, 0x72, 0x69, 0x7a,
	0x6f, 0x6e, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d,
	0x0f, 0x01, 0x01, 0xff, 0x04, 0x04, 0x03, 0x02, 0x05, 0xa0, 0x30, 0x1d,
	0x06, 0x03, 0x55, 0x1d, 0x25, 0x04, 0x16, 0x30, 0x14, 0x06, 0x08, 0x2b,
	0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, 0x06, 0x08, 0x2b, 0x06, 0x01,
	0x05, 0x05, 0x07, 0x03, 0x02, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23,
	0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0xe4, 0x2d, 0xbb, 0x91, 0x01, 0x65,
	0x26, 0x1f, 0xb4, 0x7a, 0x3f, 0xa3, 0x15, 0x25, 0xa4, 0xce, 0x8c, 0x44,
	0x33, 0x3b, 0x30, 0x3e, 0x06, 0x03, 0x55, 0x1d, 0x1f, 0x04, 0x37, 0x30,
	0x35, 0x30, 0x33, 0xa0, 0x31, 0xa0, 0x2f, 0x86, 0x2d, 0x68, 0x74, 0x74,
	0x70, 0x3a, 0x2f, 0x2f, 0x76, 0x70, 0x73, 0x73, 0x67, 0x31, 0x34, 0x32,
	0x2e, 0x63, 0x72, 0x6c, 0x2e, 0x6f, 0x6d, 0x6e, 0x69, 0x72, 0x6f, 0x6f,
	0x74, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x76, 0x70, 0x73, 0x73, 0x67, 0x31,
	0x34, 0x32, 0x2e, 0x63, 0x72, 0x6c, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d,
	0x0e, 0x04, 0x16, 0x04, 0x14, 0xc0, 0x25, 0x4d, 0xd0, 0xff, 0x42, 0x60,
	0x36, 0x05, 0x62, 0xed, 0x97, 0x96, 0x39, 0x21, 0xa1, 0x77, 0xf6, 0x36,
	0xbf, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01,
	0x01, 0x0b, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0xc6, 0x41, 0xb8,
	0x1f, 0xe4, 0x4a, 0xf7, 0x72, 0x1f, 0x27, 0x72, 0xda, 0xb3, 0x5c, 0x8b,
	0x8c, 0x38, 0x17, 0x07, 0xe9, 0x7e, 0x86, 0x64, 0x7e, 0x07, 0xe9, 0x30,
	0x43, 0x79, 0x1c, 0xfa, 0x2d, 0x19, 0x2a, 0x7c, 0x4e, 0x33, 0xfc, 0x99,
	0xca, 0x67, 0x3f, 0x72, 0x4e, 0xd9, 0xf7, 0xf1, 0xe2, 0x25, 0xbe, 0xc0,
	0x5d, 0xad, 0x31, 0x1f, 0xed, 0x7b, 0x35, 0xb1, 0x75, 0x44, 0x96, 0x5d,
	0xee, 0xdf, 0xf5, 0x1e, 0x17, 0x6f, 0x35, 0xd0, 0x58, 0x47, 0x52, 0x68,
	0xd8, 0x67, 0x13, 0x7e, 0x66, 0xa9, 0xca, 0x53, 0x38, 0x8b, 0xf6, 0x1e,
	0x68, 0x2d, 0x0f, 0x99, 0xca, 0xb0, 0x4b, 0x9c, 0x67, 0x1c, 0x16, 0x35,
	0xce, 0x27, 0x67, 0x35, 0xa2, 0x54, 0xf4, 0xad, 0x42, 0xf8, 0xcf, 0x4c,
	0x52, 0x18, 0xb7, 0x2b, 0x3d, 0xf4, 0xf3, 0xcf, 0xa0, 0xf8, 0x62, 0x69,
	0xa1, 0xa2, 0x04, 0x52, 0x6b, 0x1d, 0xd2, 0xa4, 0xbf, 0xa5, 0x10, 0x04,
	0xf6, 0x38, 0x41, 0xd8, 0x86, 0x0f, 0x5f, 0x55, 0xb7, 0x9d, 0x41, 0x4a,
	0xe0, 0x52, 0xce, 0x11, 0x94, 0x72, 0x9c, 0x89, 0xdf, 0x5b, 0x27, 0xc9,
	0xdd, 0x16, 0x3d, 0xa1, 0xfc, 0x5e, 0x92, 0xf0, 0x79, 0x3f, 0x4c, 0x02,
	0xfc, 0x6c, 0x33, 0xb5, 0x5c, 0xea, 0x6f, 0xef, 0xbc, 0xc2, 0x38, 0x7f,
	0xa1, 0x6c, 0xd4, 0x5d, 0x23, 0x4a, 0x7c, 0xcb, 0x90, 0x83, 0xb5, 0x93,
	0x47, 0xd9, 0xbf, 0x0b, 0x52, 0xcc, 0x40, 0xd6, 0x9f, 0xae, 0x0a, 0x1b,
	0x7c, 0x8e, 0x4a, 0xa2, 0x0a, 0x7e, 0x8e, 0xc6, 0x4b, 0x6c, 0x82, 0x34,
	0x7f, 0x61, 0x5b, 0x7d, 0x78, 0xfe, 0x37, 0x20, 0x6b, 0x4d, 0x9b, 0xc9,
	0x23, 0x7c, 0x72, 0xb6, 0xc1, 0xb3, 0x91, 0x14, 0x76, 0x20, 0xca, 0x14,
	0xb7, 0x00, 0x50, 0x12, 0xab, 0xe1, 0x17, 0x39, 0xce, 0xda, 0x10, 0x1f,
	0x43
};
#endif

#endif
