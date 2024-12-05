# Dynamic Memory Allocator 구현 프로젝트

## 📌 프로젝트 개요
C언어로 동적 메모리 할당기(malloc, free, realloc)를 구현하여 메모리 관리 시스템을 최적화한 프로젝트입니다.

## ⏱️ 개발 기간
2024.08.15 - 2024.08.22 (1주)

## 💻 개발 환경
- Language: C
- System: Linux

## 🏆 주요 기술 성과
- Malloc Lab Score 86점 달성
- First-fit 대비 할당 속도 개선
- 블록당 메모리 효율성 향상

## 🔍 핵심 최적화 전략

### 할당 속도 개선
- Explicit Free List 구현으로 빠른 가용 블록 탐색
- Next-fit 정책 도입으로 First-fit의 긴 탐색 시간 문제 해결
- 블록 재사용성 향상으로 할당/해제 성능 개선

### 메모리 효율성 향상
- Footer-less 설계로 Header-Footer 구조의 메모리 오버헤드 제거
- 최적화된 블록 구조로 메모리 단편화 감소
- 효율적인 메모리 정렬 구현

### 구현 기술
- Explicit List 기반 가용 블록 관리
- Footer-less 블록 구조 설계
- Next-fit 할당 정책
- 블록 분할 및 병합 최적화

## 🚀 도전 과제 해결
- First-fit의 순차 탐색 비효율성 극복
- Header-Footer 구조의 메모리 낭비 문제 해결
- 할당/해제 성능과 메모리 효율성의 균형점 도출

## 🌱 학습 성과
- 저수준 메모리 관리 시스템 설계
- 메모리 최적화 전략 수립
- 시스템 성능 분석 및 개선





#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################

***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h

