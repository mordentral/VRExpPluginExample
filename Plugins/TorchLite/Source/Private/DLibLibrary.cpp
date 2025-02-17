// Fill out your copyright notice in the Description page of Project Settings.
#include "DLibLibrary.h"

#pragma warning(push)
#pragma warning(disable: 4800)
#pragma warning(disable: 4668)
#pragma warning(disable: 4686)
#pragma warning(disable: 4541)
#pragma warning(disable: 4702)
#pragma push_macro("check")
#undef check
#pragma push_macro("ensure")
#undef ensure
//THIRD_PARTY_INCLUDES_START
#include <torch/script.h>
#include <torch/torch.h>
//THIRD_PARTY_INCLUDES_END
#pragma pop_macro("ensure")
#pragma pop_macro("check")
#pragma warning(pop)



FString  UDLibLibrary::GetTestString()
{
	torch::Tensor tensor = torch::eye(3);

	struct Net : torch::nn::Module {
		Net(int64_t N, int64_t M) {
			W = register_parameter("W", torch::randn({ N, M }));
			b = register_parameter("b", torch::randn(M));
		}
		torch::Tensor forward(torch::Tensor input) {
			return torch::addmm(b, input, W);
		}
		torch::Tensor W, b;
	};

	return FString();
}
