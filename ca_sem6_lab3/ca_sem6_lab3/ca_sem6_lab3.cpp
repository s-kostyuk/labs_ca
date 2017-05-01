// ca_sem6_lab3.cpp: определяет точку входа для консольного приложения.
//

/*****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

/*****************************************************************************/

bool check_is_32();
bool check_cpuid_supported();
int32_t get_max_cpuid_value();
bool get_vendor( char * _pVendorString );
int32_t get_signature();
int32_t get_extended_info();
bool get_feature_flags( int32_t * _flags1, int32_t * _flags2 );

/*****************************************************************************/

int main()
{
	
	/*-------------------------------------------------------------------*/
	
	int32_t flags1, flags2;

	bool is_32 = check_is_32();
	bool cpuid_supported = check_cpuid_supported();
	int32_t max_cpuid_value = get_max_cpuid_value();
	int32_t signature = get_signature();
	int32_t extended_info = get_extended_info();
	
	bool feature_flags_got = get_feature_flags( &flags1, &flags2 );
	
	/*-------------------------------------------------------------------*/

	if ( is_32 ) {
		printf( "This processor supports 32-bit instructions\n" );
	}
	else
	{
		printf( "This processor is 16-bit\n" );
	}

	/*-------------------------------------------------------------------*/

	if ( cpuid_supported ) {
		printf( "This processor supports CPUID instruction\n" );
		printf( "Max CPUID input value: %#x\n", max_cpuid_value ); // printout in hex (like 0x00ff00)
	}
	else
	{
		printf( "There is no CPUID support for this processor\n" );
		return 0;
	}

	/*-------------------------------------------------------------------*/

	const size_t VENDOR_STRING_SIZE = 13; // 12 characters + null-terminator

	char vendor[ VENDOR_STRING_SIZE ];
	vendor[ VENDOR_STRING_SIZE - 1 ] = '\0';

	if ( get_vendor( vendor ) ) {
		printf( "CPU vendor is: %s\n", vendor );
	}
	else
	{
		printf( "Unable to get CPU vendor" );
	}

	/*-------------------------------------------------------------------*/

	if ( signature >= 0 ) {
		printf( "CPU signature: %#x\n", signature );
	}
	else {
		printf( "Failed to get signature\n" );
	}

	/*-------------------------------------------------------------------*/

	if ( extended_info >= 0 ) {
		printf( "Extended info: %#x\n", extended_info );
	}
	else {
		printf( "Failed to get extended info\n" );
	}

	/*-------------------------------------------------------------------*/

	if ( feature_flags_got ) {
		printf( "Feature flags: %#x %#x\n", flags1, flags2 );
	}
	else {
		printf( "Failed to get feature flags\n" );
	}

	/*-------------------------------------------------------------------*/

    return 0;

	/*-------------------------------------------------------------------*/
}

/*****************************************************************************/

bool check_is_32() {
	/*
	1. Используя регистр флагов, необходимо убедиться в наличии 32-
	разрядного процессора в системе.
	*/
	bool is_32;

	_asm {
		mov is_32, 0; сбрасываем результат

		; save_original:
		pushf; ( Сохранить исходное состояние регистра флагов )

			; collect_detect_data:
		pushf; Скопировать регистр флагов...
			pop ax; ...в регистр AX
			xor ah, 11110000b; Поменять значение старших 4 битов
			push ax; Скопировать регистр AX
			popf; ...в регистр флагов
			pushf; Скопировать регистр флагов...
			pop bx; ...в регистр BX

			; restore_original:
		popf; ( Восстановить исходное состояние регистра флагов )

			; analyze_32_bit:
		xor ah, bh; AH = 0 ( биты в регистре флагов не поменялись ) → 808x - 80286, иначе 80386 +
			mov is_32, ah; save results to boolean value
	}

	return is_32;
}

/*****************************************************************************/

bool check_cpuid_supported() {
	/*
	2.1. Убедиться в поддержке команды CPUID (посредством бита ID
	регистра EFLAGS)...
	*/
	if ( check_is_32() == false ) {
		return false;
	}

	bool cpuid_supported;

	_asm {
		mov cpuid_supported, 0; сбрасываем результат

		; check_cpuid_support:
		; если можно установить и сбросить 21 - й бит ID в EFLAGS → CPUID подерживается
			; иначе - поддержка отсутствует
			; source: Intel SDM, Vol. 1 3 - 17

			; save_original:
		pushfd; Скопировать оригинальное состояние регистра флагов

			pushfd; Скопировать регистр флагов...
			pop eax; ...в регистр EAX
			or eax, 200000h; установить 21 - й бит( флаг ID ) в 1
			; 200000h - единица, сдвинутая на 21 разряд влево

			push eax; Скопировать регистр EAX...
			popfd; ...в регистр флагов

			pushf; Скопировать регистр флагов...
			pop bx; ...в регистр EBX

			; restore_original:
		popfd; ( Восстановить исходное состояние регистра флагов )

			xor eax, ebx; EAH = 0 ( биты в регистре флагов не поменялись ) → CPUID не подерживается, иначе - поддерживается

			jz exit; если флаг ID = 0 → CPUID не поддерживается, можно выходить
			mov cpuid_supported, 1; иначе - поддержка присутствует

			exit :
	}

	return cpuid_supported;
}

/*****************************************************************************/

int32_t get_max_cpuid_value() {
	/*
	2.2. ...и определить максимальное значение параметра ее (CPUID) вызова.
	*/
	if ( check_cpuid_supported() == false ) {
		return -1;
	}

	int32_t max_cpuid_value;

	_asm {
		; get_max_cpuid_value
		mov EAX, 00h; установка входного параметра
		cpuid; получение выходных параметров, соответствующих входному
		mov max_cpuid_value, eax; сохранение максимального входного параметра
		; в EBX : EDX:ECX – строка идентификации производителя( ANSI )
	}

	return max_cpuid_value;
}

/*****************************************************************************/

bool get_vendor( char * _pVendorString ) {
	/*
	3. Получить строку идентификации производителя процессора и
	сохранить ее в памяти.
	*/
	if ( check_cpuid_supported() == false ) {
		return false;
	}

	_asm {
		mov EAX, 00h; установка входного параметра

		cpuid; получение выходных параметров, соответствующих входному
		; В EBX : EDX:ECX получим строку идентификации производителя( ANSI )

		; адрес начала массива символов сохраняем в регистр EDI
		mov edi, _pVendorString;

		; копируем 12 байт строки идентификации производителя
			mov[ edi ], EBX; копируем первые 4 символа
			mov[ edi + 4 ], EDX; копируем вторые 4 символа
			mov[ edi + 8 ], ECX; копируем последние 4 символа
	}

	return true;
}

/*****************************************************************************/

int32_t get_signature() {
	/*
	4.1. Получить сигнатуру процессора и определить его модель, семейство и т.п.
	*/
	if ( check_cpuid_supported() == false ) {
		return -1;
	}

	int32_t signature;

	_asm {
		mov EAX, 01h; установка входного параметра
		cpuid; получение выходных параметров, соответствующих входному
		mov signature, eax; сохранение сигнатуры
	}

	return signature;
}

/*****************************************************************************/

int32_t get_extended_info() {
	/*
	4.2. Выполнить анализ дополнительной информации о процессоре.
	*/
	if ( check_cpuid_supported() == false ) {
		return -1;
	}

	int32_t extended_info;

	_asm {
		mov EAX, 01h; установка входного параметра
		cpuid; получение выходных параметров, соответствующих входному
		mov extended_info, EBX; сохранение расширенной информации
	}

	return extended_info;
}

/*****************************************************************************/

bool get_feature_flags( int32_t * _flags1, int32_t * _flags2 ) {
	/*
	5. Получить флаги свойств. Составить список поддерживаемых
	процессором свойств.
	*/
	if ( check_cpuid_supported() == false ) {
		return false;
	}

	_asm {
		mov EAX, 01h; установка входного параметра
		cpuid; получение выходных параметров, соответствующих входному
		mov esi, _flags1;
		mov edi, _flags2;
		mov [ esi ], EDX; сохранение флагов свойств из EDX
		mov [ edi ], ECX; сохранение флагов свойств из ECX
	}

	return true;
}

/*****************************************************************************/