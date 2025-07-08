# Sistema de Gerenciamento de Farmácia

## Descrição

Este projeto é um sistema acadêmico desenvolvido em C para gerenciamento de uma farmácia, com foco principal na utilização de **Árvore B** como estrutura de dados para otimizar a busca de informações. O projeto demonstra a aplicação prática de uma estrutura eficiente para indexação e recuperação de dados, fundamental para sistemas que trabalham com grandes volumes de informações.

## Objetivo

O objetivo principal deste trabalho é apresentar a implementação e o funcionamento da Árvore B como índice primário para acelerar operações de busca em arquivos de dados relacionados a medicamentos. Trata-se de um projeto acadêmico que une conceitos de estruturas de dados e manipulação de arquivos para melhorar a eficiência das consultas.

## Funcionalidades

- Armazenamento de dados sobre produtos farmacêuticos em arquivo texto.
- Implementação de Árvore B para indexação e busca rápida por código dos medicamentos.
- Manipulação e manutenção da árvore em disco para garantir a persistência dos índices.
- Busca eficiente mesmo em grandes volumes de dados, graças à Árvore B.

## Tecnologias e Ferramentas

- Linguagem: C
- Estrutura de Dados: Árvore B (B-Tree)
- Manipulação de arquivos para armazenamento de dados e índices.

## Estrutura do Projeto

- `litale.cpp` — Arquivo principal contendo a lógica do sistema e implementação da Árvore B.
- `dados.txt` — Arquivo que armazena os dados dos medicamentos.
- `indice.idx` — Arquivo que mantém o índice da Árvore B para buscas rápidas.

## Uso

O sistema é executado via console, onde o usuário pode realizar operações de busca, inserção e consulta de medicamentos através do índice construído pela Árvore B.

## Considerações Finais

Este projeto serve como uma aplicação prática dos conceitos estudados em disciplinas de estruturas de dados e sistemas de informação, demonstrando como estruturas de dados avançadas podem ser usadas para otimizar sistemas de gerenciamento reais.
